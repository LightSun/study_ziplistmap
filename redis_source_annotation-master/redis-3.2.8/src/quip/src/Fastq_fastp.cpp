#include "Fastq_fastp.h"
#include "quip.h"
#include <iostream>
#include <fstream>

#define ASSERT(x, fmt, ...) \
do{\
    if(!(x)){\
        /*fprintf(stderr, fmt, ##__VA_ARGS__);*/\
        char buf[1024];\
        snprintf(buf, 1024, fmt, ##__VA_ARGS__);\
        std::cout << buf << std::endl;\
        abort();\
    }\
}while(0);

using CString = const std::string&;
using String = std::string;
using StringP = std::string*;

using namespace fastp;

namespace fastp {

struct Read{
    String name;
    String seq;
    String strand;
    String qua;
};

struct _Fastq_fastp_ctx{
    String inFile;
    FILE* fin {nullptr};
    quip_in_t* in {nullptr};

    short_read_t* curRead {nullptr};
    bool finished {false};

    _Fastq_fastp_ctx(CString inFile){
        this->inFile = inFile;
        fin = _quip_open_fin(inFile.data());
        ASSERT(fin, "open file failed: %s.", inFile.data());
        in = quip_in_open_file(fin, QUIP_FMT_QUIP, QUIP_FILTER_NONE, 0, NULL);
        ASSERT(in, "open qp file failed: %s", inFile.data());
    }
    ~_Fastq_fastp_ctx(){
        close();
    }
    void close(){
        if(in){
            quip_in_close(in);
            in = nullptr;
        }
        if(fin){
            fclose(fin);
            fin = nullptr;
        }
    }
    bool hasNext(){
        if(finished && !curRead){
            return false;
        }
        if(curRead != nullptr){
            return true;
        }
        curRead = quip_read(in);
        if(!curRead){
            finished = true;
            return false;
        }
        //curRead.name = String((char*)r->id.s);
        return true;
    }
    void read(StringP name, StringP seq, StringP strand, StringP quality){
        *name = String((char*)curRead->id.s);
        *seq = String((char*)curRead->seq.s);
        *quality = String((char*)curRead->qual.s);
        *strand = "+";
        curRead = nullptr;
    }
    void getBytes(size_t& bytesRead, size_t& bytesTotal){
       bytesRead = ftell(fin);
       std::ifstream is(inFile);
       is.seekg (0, is.end);
       bytesTotal = is.tellg();
    }
};
}

Fastq_fastp::Fastq_fastp(CString fn){
    m_ptr = new _Fastq_fastp_ctx(fn);
}
Fastq_fastp::~Fastq_fastp(){
    if(m_ptr){
        delete m_ptr;
        m_ptr = nullptr;
    }
}
void Fastq_fastp::getBytes(size_t& bytesRead, size_t& bytesTotal){
    m_ptr->getBytes(bytesRead, bytesTotal);
}

void Fastq_fastp::read(StringP name, StringP seq, StringP strand, StringP quality){
    m_ptr->read(name, seq, strand, quality);
}

bool Fastq_fastp::hasNextRead(){
    return m_ptr->hasNext();
}



