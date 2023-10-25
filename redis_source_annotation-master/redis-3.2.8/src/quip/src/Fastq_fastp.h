#pragma once

#include <string>

namespace fastp {

typedef struct _Fastq_fastp_ctx _Fastq_fastp_ctx;

class Fastq_fastp
{
public:
    using CString = const std::string&;
    using String = std::string;
    using StringP = std::string*;
    Fastq_fastp(CString fn);
    ~Fastq_fastp();

    void getBytes(size_t& bytesRead, size_t& bytesTotal);

    void read(StringP name, StringP seq, StringP strand, StringP quality);

    bool hasNextRead();

private:
    _Fastq_fastp_ctx* m_ptr;
};

}

