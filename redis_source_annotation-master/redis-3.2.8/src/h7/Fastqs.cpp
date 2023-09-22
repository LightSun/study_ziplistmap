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
//head: {fix(8), compress_method(4), block_count(4)}
//block: {read_count(4), hash(8), data_size(8-after_compress), data_str(N)}
namespace h7 {

#define _HMZ "mheavenz"
#define _HASH_DEFAULT 23
#define _COMP_METHOD kCompress_ZSTD
struct _BlockItem{
    uint64 hash;
    uint32 read_rc;
    String compressedData;
};
struct _ReadWriter_HMZ_ctx{
    BufferFileOutput* fos{nullptr};
    uint32 m_read_count {0};
    uint64 m_hash {_HASH_DEFAULT};
    String m_content;
    std::vector<_BlockItem> m_items;

    BufferFileOutput::Func_Buffer m_func = [this](char* data, uint64 size){
        m_hash = fasthash64(data, size, m_hash);
        m_content += String(data, size);
        return true;
    };

    _ReadWriter_HMZ_ctx(unsigned long long max_block_size){
        fos = new BufferFileOutput(max_block_size);
    }
    ~_ReadWriter_HMZ_ctx(){
        close();
    }
    bool open(CString file){
        close();
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
        auto method = std::to_string(_COMP_METHOD);
        switch (method.length()) {
        case 1:
            method = "000" + method;
            break;
        case 2:
            method = "00" + method;
            break;
        case 3:
            method = "0" + method;
            break;
        case 4:
            break;
        default:
            ASSERT(false, "compress method error.");
        }
        String str(_HMZ);
        str += method;
        str += "0000";
        ASSERT(str.length() == 16, "");
        fos->fWrite(str);
        fos->fFlush();
    }
    bool write(Read* r){
        String _str = r->toString();
        //length + data.
        String str = std::to_string(_str.length()) + _str;
        if(!fos->write(str, m_func, &m_read_count)){
           // LOGE("write read failed.\n");
            return false;
        }
        _performWriteContent();
        return true;
    }
    void end(){
        //write left content
        ASSERT(fos->flush(m_func, &m_read_count), "flush failed");
        _performWriteContent();
        fos->fFlush();
        //write block count
        fos->seek(12);
        uint32 blockCount = m_items.size();
        fos->fWrite(&blockCount, sizeof(blockCount));
        fos->fFlush();
    }
    void _performWriteContent(){
        if(m_read_count > 0){
            auto cm = CompressManager::get()->getCompressor(_COMP_METHOD);
            ASSERT(cm, "can't find compressor, you should call reg(%d,...).",
                   _COMP_METHOD);
            String out;
            {
                auto out_len = cm->compress(m_content.data(), m_content.length(), &out);
                if(out_len <= 0){
                    ASSERT(false, "compress failed");
                }
            }
            {
                _BlockItem item;
                item.read_rc = m_read_count;
                item.hash = m_hash;
                m_items.push_back(std::move(item));
            }
            //write
            fos->fWrite(&m_read_count, sizeof (m_read_count));
            fos->fWrite(&m_hash, sizeof(m_hash));
            uint64 ds = out.length();
            fos->fWrite(&ds, sizeof(ds));
            fos->fWrite(out.data(), out.length());
            {
                m_content.clear();
                m_hash = _HASH_DEFAULT;
                m_read_count = 0;
            }
        }
    }
};
}

ReadWriter_HMZ::ReadWriter_HMZ(CString file, unsigned long long max_block_size){
    m_ptr = new _ReadWriter_HMZ_ctx(max_block_size);
    ASSERT(m_ptr->open(file), "open file faild. %s", file.data());
}
ReadWriter_HMZ::~ReadWriter_HMZ(){
    if(m_ptr){
        delete m_ptr;
        m_ptr = nullptr;
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


//--------------------------------------------
static std::map<char, int> _sIndexMap = {
    {'N', 10000},
    {'A', 01000},
    {'C', 00100},
    {'T', 00010},
    {'G', 00001},
};
static inline int _GetFlag(char ch){
    auto it = _sIndexMap.find(ch);
    if(it != _sIndexMap.end()){
        return it->second;
    }
    return _sIndexMap['N'];
}
//NACTG 11111
//XXXX -> 1111
String Read::toString(){
    std::stringstream ss;
    ss << (uint16)name.length();
    ss << name;
    ss << (uint32)qual.length();
    ss << qual;
    ss << (char)lastChar;
    char* chs = seq.data();
    uint32 len = seq.length();
    int left = len % 5;
    char buf_chs[5];
    for(uint32 i = 0 ; i < len - 5 ; i += 5){
        //buf_chs[i] =
        _GetFlag(buf_chs[i]);
    }
}
void Read::fromString(CString str){

}
Fastqs::Fastqs()
{

}
