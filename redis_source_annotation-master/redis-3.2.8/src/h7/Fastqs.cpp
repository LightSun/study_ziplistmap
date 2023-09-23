#include "Fastqs.h"

#include <zlib.h>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include "common.h"
#include "FileIO.h"
#include "hash.h"
#include "CompressManager.h"
#include "DNACompressManager.h"
#include "ByteBufferIO.h"

#include "kseq.h"
KSEQ_INIT(gzFile, gzread)

#define GET_SEQ_STR(val) (val.l > 0 ? std::string(val.s,val.l) : "")

using namespace h7;

namespace h7 {
struct _ReadReader_GZ_ctx{
    gzFile fp {nullptr};
    kseq_t *seq {nullptr};
    FILE* mfile ;

     void open(CString file){
         ASSERT(fp == nullptr, "already open a file.");
         gzFile fp;
         FILE* mfile = fopen64(file.data(), "rb");
         fp = gzdopen(fileno(mfile), "r");
         seq = kseq_init(fp);
     }
     void close(){
         if(fp != nullptr){
             kseq_destroy(seq);
             gzclose(fp);
             fclose(mfile);
             fp = nullptr;
         }
     }

     bool nextRead(Read* r){
         if (kseq_read(seq) >= 0){
            r->name = GET_SEQ_STR(seq->name);
            r->seq = GET_SEQ_STR(seq->seq);
            r->qual = GET_SEQ_STR(seq->qual);
            r->comment = GET_SEQ_STR(seq->comment);
            r->lastChar = seq->last_char;
            return true;
         }
         return false;
     }
};
}

ReadReader_GZ::ReadReader_GZ(CString file){
    m_ptr = new _ReadReader_GZ_ctx();
    m_ptr->open(file);
}
ReadReader_GZ::~ReadReader_GZ(){
    if(m_ptr){
        m_ptr->close();
        delete m_ptr;
        m_ptr = nullptr;
    }
}
bool ReadReader_GZ::nextRead(Read* r){
    return m_ptr->nextRead(r);
}
//------------------------------------------
//head. blocks
//head: {fix(8), compress_method(2), dna_compress_method(2), block_count(4)}
//block: {hash(8), read_count(4), len_names(4 * read_count), len_seqs(4 * read_count),
//                                  seq_data_size(8-after_compress), seq_data_str(N),
//                                  non_seq_data_size(8-after_compress), non_seq_data_str(N)}
namespace h7 {

#define _HMZ "mheavenz"
#define _HASH_DEFAULT 23

using CompressParams = ReadWriter_HMZ::Params;
using ListI = std::vector<int>;
using ListS = std::vector<String>;

struct _BlockItem{
    CompressParams* m_params;
    uint64 hash {_HASH_DEFAULT};
    uint32 read_count {0};
    ListI len_names;
    ListI len_seqs;
    String seqStrs;
    String nonSeqStrs;

    _BlockItem(CompressParams* p): m_params(p){}

    String compress(){
        ByteBufferOut bos;
        {
        String seqCodes;
        auto len = getDnaCompressor()->compress(seqStrs.data(), seqStrs.length(), &seqCodes);
        ASSERT(len > 0, "_BlockItem >> dna compress failed.");
        bos.prepareBuffer(seqStrs.length());
        bos.putULong(hash);
        bos.putUInt(read_count);
        bos.putListInt(len_names);
        bos.putListInt(len_seqs);
        bos.putString64(seqCodes);
        }
        String out;
        auto ret = getCompressor()->compress(nonSeqStrs.data(), nonSeqStrs.length(), &out);
        ASSERT(ret > 0, "_BlockItem >> compress failed !");
        bos.putString64(out);
        return bos.bufferToString();
    }
    std::shared_ptr<Compressor> getCompressor(){
        return CompressManager::get()->getCompressor(m_params->compress_method);
    }
    std::shared_ptr<Compressor> getDnaCompressor(){
        return DNACompressManager::get()->getCompressor(m_params->dna_compress_method);
    }
};
using SpBlockItem = std::shared_ptr<_BlockItem>;

//compress by diff category, like quality,name,last_char.   seqs
struct _ReadWriter_HMZ_ctx{
    BufferFileOutput* fos{nullptr};
    uint32 m_max_read_count {0};
    uint32 m_block_count {0};
    SpBlockItem m_curItem;

    CompressParams m_params;

    _ReadWriter_HMZ_ctx(unsigned int max_read_count):m_max_read_count(max_read_count){
        m_params.compress_method = kCompress_ZSTD;
        m_params.dna_compress_method = kDNA_COMPRESS_HUFFMAN;
        uint64 max = ~0;
    }
    ~_ReadWriter_HMZ_ctx(){
        close();
    }
    CompressParams* getParams(){
        return &m_params;
    }
    SpBlockItem getCurItem(){
        return m_curItem;
    }
    bool open(CString file){
        close();
        fos = new BufferFileOutput(m_max_read_count);
        fos->open(file);
        return fos->is_open();
    }
    void close(){
        if(fos){
            fos->close();
            delete fos;
            fos = nullptr;
        }
    }
    void begin(){
        //write head
        ByteBufferOut bos;
        bos.prepareBuffer(16);
        bos.setBufferAutoInc(false);
        bos.putRawString(_HMZ);
        bos.putUShort((uint16)m_params.compress_method);
        bos.putUShort((uint16)m_params.dna_compress_method);
        bos.putUInt(0);

        auto str = bos.bufferToString();
        ASSERT(str.length() == 16, "head must be 16.");
        fos->fWrite(str);
        fos->fFlush();
    }
    bool write(Read* r){
        ASSERT(r->seq.length() == r->qual.length(), "must seq.len == qual.len.");
        auto curItem = getCurItem();
        if(!curItem){
            curItem = m_curItem = std::make_shared<_BlockItem>(&m_params);
            m_block_count ++;
        }
        curItem->hash = fasthash64(r->seq.data(), r->seq.length(), m_curItem->hash);
        curItem->read_count ++;
        curItem->len_names.push_back(r->name.length());
        curItem->len_seqs.push_back(r->seq.length());
        curItem->seqStrs.append(r->seq);
        curItem->nonSeqStrs.append(r->nonSeqToString());
        if(curItem->read_count >= m_max_read_count){
            auto str = curItem->compress();
            ASSERT(fos->fWrite(str.data(), str.length()), "write >> write block failed");
            fos->fFlush();
            m_curItem = nullptr;
        }
        return true;
    }
    void end(){
        //write left content
        auto curItem = getCurItem();
        if(curItem){
            auto str = curItem->compress();
            ASSERT(fos->fWrite(str.data(), str.length()), "end >>write block failed");
            fos->fFlush();
            m_curItem = nullptr;
        }
        //write block count
        fos->seek(12);
        fos->fWrite(&m_block_count, sizeof(m_block_count));
        fos->fFlush();
    }
};
}

ReadWriter_HMZ::ReadWriter_HMZ(CString file, unsigned int max_read_count){
    m_ptr = new _ReadWriter_HMZ_ctx(max_read_count);
    ASSERT(m_ptr->open(file), "open file faild. %s", file.data());
}
ReadWriter_HMZ::~ReadWriter_HMZ(){
    if(m_ptr){
        delete m_ptr;
        m_ptr = nullptr;
    }
}
void ReadWriter_HMZ::setParams(const Params& p){
    if(p.compress_method > 0){
        m_ptr->m_params.compress_method = p.compress_method;
    }
    if(p.dna_compress_method > 0){
        m_ptr->m_params.dna_compress_method = p.dna_compress_method;
    }
}
void ReadWriter_HMZ::begin(){
    m_ptr->begin();
}
bool ReadWriter_HMZ::write(Read* r){
    return m_ptr->write(r);
}
void ReadWriter_HMZ::end(){
    m_ptr->end();
}
//-------------------------------------------
struct _ReadReader_HMZ_ctx{

};
ReadReader_HMZ::ReadReader_HMZ(CString file){

}
ReadReader_HMZ::~ReadReader_HMZ(){

}
bool ReadReader_HMZ::nextRead(Read* r){

}


//--------------------------------------------
//NACTG 11111
//XXXX -> 1111
String Read::toString(){
    std::stringstream ss;
    ss << "{name=";
    ss << name << ", ";
    ss << "seq=";
    ss << seq << ", ";
    ss << "qual=";
    ss << qual << ", ";
    ss << (char)lastChar;
    ss << "}";
    return ss.str();
}
String Read::nonSeqToString(){
    return name + qual + String(1, (char)lastChar);
}

Fastqs::Fastqs()
{

}
