#include "Fastqs.h"

#include <zlib.h>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <list>

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

    void toReads(std::list<Read>& out){
        uint64 offset_seq = 0;
        uint64 offset_nonseq = 0;
        for(int i = 0 ; i < read_count; ++i){
            int& len_name = len_names[i];
            int& len_seq = len_seqs[i];
            Read r;
            r.seq = seqStrs.substr(offset_seq, len_seq);
            offset_seq += r.seq.length();
            //
            r.name = nonSeqStrs.substr(offset_nonseq, len_name);
            offset_nonseq += r.name.length();
            r.qual = nonSeqStrs.substr(offset_nonseq, len_seq);
            offset_nonseq += r.qual.length();
            r.lastChar = nonSeqStrs.data()[offset_nonseq];
            offset_nonseq += 1;
            out.push_back(std::move(r));
        }
    }
    void addRead(Read* r){
        hash = fasthash64(r->seq.data(), r->seq.length(), hash);
        read_count ++;
        len_names.push_back(r->name.length());
        len_seqs.push_back(r->seq.length());
        seqStrs.append(r->seq);
        nonSeqStrs.append(r->nonSeqToString());
    }

    void decompress(CString in){
        ByteBufferIO bis((String*)&in);
        hash = bis.getULong();
        read_count = bis.getUInt();
        bis.getListInt(len_names);
        bis.getListInt(len_seqs);
        {
        String seqCodes = bis.getString64();
        auto len = getDnaCompressor()->deCompress(seqCodes, &seqStrs);
        ASSERT(len > 0, "_BlockItem >> dna deCompress failed.");
        }

        {
        String nonSeqs_de = bis.getString64();
        auto ret = getCompressor()->deCompress(nonSeqs_de, &nonSeqStrs);
        ASSERT(ret > 0, "_BlockItem >> deCompress failed !");
        }
    }
    String compress(){
        ByteBufferOut bos;
        {
        String seqCodes;
        auto len = getDnaCompressor()->compress(seqStrs, &seqCodes);
        ASSERT(len > 0, "_BlockItem >> dna compress failed.");
        bos.prepareBuffer(seqStrs.length());
        bos.putULong(hash);
        bos.putUInt(read_count);
        bos.putListInt(len_names);
        bos.putListInt(len_seqs);
        bos.putString64(seqCodes);
        }
        String out;
        auto ret = getCompressor()->compress(nonSeqStrs, &out);
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
        curItem->addRead(r);

        if(curItem->read_count >= m_max_read_count){
            _writeItem(curItem);
        }
        return true;
    }
    void end(){
        //write left content
        auto curItem = getCurItem();
        if(curItem){
            _writeItem(curItem);
        }
        //write block count
        fos->seek(12);
        fos->fWrite(&m_block_count, sizeof(m_block_count));
        fos->fFlush();
    }
    void _writeItem(SpBlockItem curItem){
        auto str = curItem->compress();
        uint64 len = str.length();
        ASSERT(fos->fWrite(&len, sizeof(len)), "write >> write block_len failed");
        ASSERT(fos->fWrite(str.data(), str.length()), "end >>write block failed");
        fos->fFlush();
        m_curItem = nullptr;
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
namespace h7 {
struct _ReadReader_HMZ_ctx{

    FileInput fis;
    CompressParams m_params;
    uint32 m_block_count {0};
    uint32 m_dec_block_count {0};
    std::list<Read> m_reads;

    _ReadReader_HMZ_ctx(CString file){
        fis.open(file);
        ASSERT(fis.is_open(), "open file failed: %s", file.data());
        auto head = fis.read(16);
        ByteBufferIO bis(&head);
        ASSERT(bis.getRawString(8) == _HMZ, "read failed by file format error.");
        m_params.compress_method = bis.getUShort();
        m_params.dna_compress_method = bis.getUShort();
        m_block_count = bis.getUInt();
    }

    bool nextRead(Read* r){
        if(m_reads.empty()){
            if(m_dec_block_count >= m_block_count){
                return false;
            }
            auto bl = fis.readLong();
            auto str = fis.read(bl);
            _BlockItem bi(&m_params);
            bi.decompress(str);
            bi.toReads(m_reads);
            m_dec_block_count ++;
        }
        *r = m_reads.front();
        m_reads.pop_front();
        return true;
    }
};
}
ReadReader_HMZ::ReadReader_HMZ(CString file){
    m_ptr = new _ReadReader_HMZ_ctx(file);
}
ReadReader_HMZ::~ReadReader_HMZ(){
    if(m_ptr){
        delete m_ptr;
        m_ptr = nullptr;
    }
}
bool ReadReader_HMZ::nextRead(Read* r){
    return m_ptr->nextRead(r);
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
