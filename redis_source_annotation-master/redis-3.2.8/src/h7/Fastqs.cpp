#include "Fastqs.h"

#include <zlib.h>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <list>
#include <thread>
#include <condition_variable>

#include "common.h"
#include "FileIO.h"
#include "hash.h"
#include "CompressManager.h"
#include "DNACompressManager.h"
#include "ByteBufferIO.h"
#include "CountDownLatch.h"

#include "kseq.h"
KSEQ_INIT(gzFile, gzread)

#define GET_SEQ_STR(val) (val.l > 0 ? std::string(val.s,val.l) : "")

using namespace h7;

namespace h7 {
struct _ReadReader_GZ_ctx{
    gzFile fp {nullptr};
    kseq_t *seq {nullptr};
    FILE* mfile ;

    ~_ReadReader_GZ_ctx(){
        close();
    }

     void open(CString file){
         ASSERT(fp == nullptr, "already open a file.");
         gzFile fp;
         FILE* mfile = fopen64(file.data(), "rb");
         ASSERT(mfile != nullptr, "open file failed. %s", file.data());
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
    ListI len_comments;
    String seqStrs;
   // String qualStrs;
    String nonSeqStrs;

    _BlockItem(CompressParams* p): m_params(p){}

    void toReads(std::list<Read>& out){
        uint64 offset_seq = 0;
        uint64 offset_nonseq = 0;
        for(uint32 i = 0 ; i < read_count; ++i){
            int& len_name = len_names[i];
            int& len_seq = len_seqs[i];
            int& len_comment = len_comments[i];
            Read r;
            r.seq = seqStrs.substr(offset_seq, len_seq);
            //r.qual = qualStrs.substr(offset_seq, len_seq);
            offset_seq += len_seq;
            //name + qual? + commont + lastChar
            r.name = nonSeqStrs.substr(offset_nonseq, len_name);
            offset_nonseq += len_name;

            r.qual = nonSeqStrs.substr(offset_nonseq, len_seq);
            offset_nonseq += len_seq;

            r.comment = nonSeqStrs.substr(offset_nonseq, len_comment);
            offset_nonseq += len_comment;
            r.lastChar = nonSeqStrs.data()[offset_nonseq];
            offset_nonseq += sizeof(char);
            out.push_back(std::move(r));
        }
    }
    void addRead(Read* r){
        hash = fasthash64(r->seq.data(), r->seq.length(), hash);
        read_count ++;
        len_names.push_back(r->name.length());
        len_seqs.push_back(r->seq.length());
        len_comments.push_back(r->comment.length());
        seqStrs.append(r->seq);
       // qualStrs.append(r->qual);
        nonSeqStrs.append(r->nonSeqToString());
    }

    void decompress(CString in){
        ByteBufferIO bis((String*)&in);
        hash = bis.getULong();
        read_count = bis.getUInt();
        bis.getListInt(len_names);
        bis.getListInt(len_seqs);
        bis.getListInt(len_comments);
        {
            String seqCodes = bis.getString64();
            auto len = getDnaCompressor()->deCompress(seqCodes, &seqStrs);
            ASSERT(len > 0, "_BlockItem >> dna deCompress failed.");
        }
//        {
//            String qualCodes = bis.getString64();
//            auto len = getDnaCompressor()->deCompress(qualCodes, &qualStrs);
//            ASSERT(len > 0, "_BlockItem >> dna deCompress failed.");
//        }
        {
        String nonSeqs_de = bis.getString64();
        auto ret = getCompressor()->deCompress(nonSeqs_de, &nonSeqStrs);
        ASSERT(ret > 0, "_BlockItem >> deCompress failed !");
        }
    }
    String compress(){
        ByteBufferOut bos;
        {
        bos.prepareBuffer(seqStrs.length());
        bos.putULong(hash);
        bos.putUInt(read_count);
        bos.putListInt(len_names);
        bos.putListInt(len_seqs);
        bos.putListInt(len_comments);
        }
        {
            String seqCodes;
            auto len = getDnaCompressor()->compress(seqStrs, &seqCodes);
            ASSERT(len > 0, "_BlockItem >> dna compress failed.");
            bos.putString64(seqCodes);
        }
//        {
//            String qualCodes;
//            auto len = getDnaCompressor()->compress(qualStrs, &qualCodes);
//            ASSERT(len > 0, "_BlockItem >> dna compress failed.");
//            bos.putString64(qualCodes);
//        }
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
    uint64 m_rawSize {0};
    SpBlockItem m_curItem;
    CompressParams m_params;

    _ReadWriter_HMZ_ctx(unsigned int max_read_count):m_max_read_count(max_read_count){
        m_params.compress_method = kCompress_ZSTD;
        m_params.dna_compress_method = kDNA_COMPRESS_HUFFMAN;
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
        m_rawSize = 0;
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
        m_rawSize += r->dataSize(true);

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
    m_ptr = new _ReadWriter_HMZ_ctx(max_read_count > 0 ? max_read_count : 100000);
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
unsigned long long ReadWriter_HMZ::getRawDataSize(){
    return m_ptr->m_rawSize;
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

    ss << "comment=";
    ss << comment << ", ";

    ss << "lastChar=";
    ss << (char)lastChar;
    ss << "}";
    return ss.str();
}
String Read::nonSeqToString(){
    return name + qual + comment + String(1, (char)lastChar);
}
unsigned int Read::dataSize(bool include_newLine){
    auto len =  name.length() + seq.length() + qual.length() + comment.length();
    if(include_newLine){
        len += 4;
    }
    return len;
}
//-------------------------------------
#define _LOCK(mtx) std::unique_lock<std::mutex> lock(mtx)
namespace h7 {
using SPRead = std::shared_ptr<Read>;
using SPHMZ = std::shared_ptr<ReadWriter_HMZ>;
struct _Fastqs_ctx{
    FastqIOParams m_params;
    volatile int m_cancel {0};
    volatile int m_finish {0};
    std::mutex mtx;
    std::condition_variable m_conv;
    std::list<SPRead> m_reads;

    SPHMZ m_writer;
    CountDownLatch* m_cdl {nullptr};

    _Fastqs_ctx(const FastqIOParams& ps):m_params(ps){}
    ~_Fastqs_ctx(){
        if(m_cdl){
            m_cdl->await();
            delete m_cdl;
            m_cdl = nullptr;
        }
    }
    uint32 getBatchReadCount(){
        return m_params.batch_read_count;
    }
    void cancel(){
        if(h_atomic_get(&m_finish) == 0){
            h_atomic_set(&m_cancel, 1);
        }
    }
    void finish(){
        h_atomic_set(&m_finish, 1);
    }
    void openHMZOutput(CString out){
        ASSERT(m_writer == nullptr, "already open a writer.");
        m_writer = std::make_shared<ReadWriter_HMZ>(out, m_params.max_read_count);
        {
            ReadWriter::Params ps;
            ps.compress_method = m_params.compress_method;
            ps.dna_compress_method = m_params.dna_compress_method;
            m_writer->setParams(ps);
        }
        m_cdl = new CountDownLatch(1);
        _start();
    }

    void _start(){
        std::thread thd([this](){
            m_writer->begin();
            while (h_atomic_get(&m_cancel) == 0) {
                auto reads = _pollReads(m_params.batch_write_count);
                if(reads.size() > 0){
                    do{
                        auto read = reads.front();
                        reads.pop_front();
                        m_writer->write(read.get());
                    }while (reads.size() > 0);
                }else{
                    if(h_atomic_get(&m_finish) == 1){
                        if(_getCurReadCount() == 0){
                            m_writer->end();
                            m_writer = nullptr;
                            break;
                        }else{
                            continue;
                        }
                    }else{
                        _LOCK(mtx);
                        m_conv.wait(lock);
                    }
                }
            }
            if(m_cdl){
                m_cdl->countDown();
            }
            LOGI("_Fastqs_ctx >> write thread exist!");
        });
        thd.detach();
    }
    uint32 _getCurReadCount(){
        _LOCK(mtx);
        return m_reads.size();
    }
    std::list<SPRead> _pollReads(int count){
        std::list<SPRead> ret;
        _LOCK(mtx);
        if(m_reads.size() == 0){
            return ret;
        }
        if(count > 0){
            for(int i = 0 ; i < count ; ++i){
                ret.push_back(m_reads.front());
                m_reads.pop_front();
            }
        }else{
            ret = m_reads;
            m_reads.clear();
        }
        return ret;
    }
    void addReads(const std::list<SPRead>& reads){
        _LOCK(mtx);
        m_reads.insert(m_reads.end(), reads.begin(), reads.end());
        m_conv.notify_all();
    }
};
}

void Fastqs::RegAll(){
    CompressManager::get()->regAll();
    DNACompressManager::get()->regAll();
}
void Fastqs::Gz2Hmz(CString in, CString out, const FastqIOParams& p){
    ReadWriter_HMZ writer(out, p.max_read_count);
    {
        ReadWriter::Params ps;
        ps.compress_method = p.compress_method;
        ps.dna_compress_method = p.dna_compress_method;
        writer.setParams(ps);
    }
    writer.begin();
    //
    ReadReader_GZ reader(in);
    Read r;
    while (reader.nextRead(&r)) {
        ASSERT(writer.write(&r), "write read failed. file = %s", out.data());
    }
    writer.end();
    LOGI("file >> raw_file_size = %llu\n", writer.getRawDataSize());
}
void Fastqs::CompressFile(CString in, CString out){
    auto cm = CompressManager::get()->getCompressor(kCompress_ZSTD);
    String str_in;
    {
    FileInput fis(in);
    ASSERT(fis.is_open(), "open file in failed. %s", in.data());
    ASSERT(fis.read2Str(str_in), "read failed.");
    LOGI("read file done: %s\n", in.data());
    }
    {
        String str_out;
        auto ret = cm->compress(str_in, &str_out);
        ASSERT(ret > 0, "compress failed.");
        FileOutput fos(out);
        ASSERT(fos.is_open(), "open file out failed. %s", out.data());
        uint64 len = str_out.length();
        fos.write(&len, sizeof (len));
        fos.write(str_out.data(), str_out.length());
        fos.flush();
    }
}

Fastqs::Fastqs(const FastqIOParams& ps){
    m_ptr = new _Fastqs_ctx(ps);
}
Fastqs::~Fastqs(){
    if(m_ptr){
        m_ptr->cancel();
        delete m_ptr;
        m_ptr = nullptr;
    }
}
void Fastqs::gz2Hmz(CString in, CString out){
    m_ptr->openHMZOutput(out);
    //
    ReadReader_GZ reader(in);
    std::list<SPRead> _reads;
    SPRead _read = std::make_shared<Read>();
    while (reader.nextRead(_read.get())) {
        _reads.push_back(_read);
        _read = std::make_shared<Read>();
        if(_reads.size() >= m_ptr->getBatchReadCount()){
            m_ptr->addReads(_reads);
            _reads.clear();
        }
    }
    if(_reads.size() > 0){
        m_ptr->addReads(_reads);
    }
    m_ptr->finish();
}
