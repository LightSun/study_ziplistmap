#pragma once

#include <string>
#include <vector>

namespace h7 {

#ifndef String
using String = std::string;
#endif
#ifndef CString
using CString = const std::string&;
#endif

struct Read{
    std::string name;
    std::string seq;
    std::string qual;
    std::string comment;
    int lastChar;

    String toString();
    String nonSeqToString();
    unsigned int dataSize(bool include_newLine = false);
};

class ReadReader{
public:
    virtual bool nextRead(Read* r) = 0;

    //if not null , need delete.
    Read* nextRead(){
        auto r = new Read();
        if(nextRead(r)){
            return r;
        }
        delete r;
        return nullptr;
    }
};

class ReadWriter{
public:
    struct Params{
        //0 means default
        int compress_method {0};
        int dna_compress_method {0};
    };
    virtual void setParams(const Params&) = 0;
    virtual void begin() = 0;
    virtual bool write(Read* r) = 0;
    virtual void end() = 0;

    virtual unsigned long long getRawDataSize(){
        return 0;
    }
    virtual unsigned int getReadCount(){
        return 0;
    }
};

//-------------------------------------------------
typedef struct _ReadReader_GZ_ctx _ReadReader_GZ_ctx;
class ReadReader_GZ: public ReadReader{
public:
    ReadReader_GZ(CString file);
    ~ReadReader_GZ();
    bool nextRead(Read* r) override;

private:
    _ReadReader_GZ_ctx* m_ptr {nullptr};
};
//--------------------------------
typedef struct _ReadWriter_HMZ_ctx _ReadWriter_HMZ_ctx;
class ReadWriter_HMZ: public ReadWriter{
public:
    /// o means default
    ReadWriter_HMZ(CString file, unsigned int max_read_count = 0);
    ~ReadWriter_HMZ();
    void setParams(const Params&) override;
    void begin() override;
    bool write(Read* r) override;
    void end() override;

    unsigned long long getRawDataSize() override;

private:
    _ReadWriter_HMZ_ctx* m_ptr{nullptr};
};

typedef struct _ReadReader_HMZ_ctx _ReadReader_HMZ_ctx;
class ReadReader_HMZ: public ReadReader{
public:
    ReadReader_HMZ(CString file);
    ~ReadReader_HMZ();
    bool nextRead(Read* r) override;

private:
    _ReadReader_HMZ_ctx* m_ptr {nullptr};
};

//----------------------------
typedef struct _ReadReader_QP_ctx _ReadReader_QP_ctx;
class ReadReader_QP: public ReadReader{
public:
    ReadReader_QP(CString file);
    ~ReadReader_QP();
    bool nextRead(Read* r) override;

private:
    _ReadReader_QP_ctx* m_ptr {nullptr};
};

//-------------------------------------
struct FastqIOParams{
    using uint32 = unsigned int;
    //0 for default.
    int compress_method {0};
    int dna_compress_method {0};
    /// max read count for one block.
    uint32 max_read_count {0};
    uint32 batch_read_count = 1000;
    uint32 batch_write_count = 1000;
};
typedef struct _Fastqs_ctx _Fastqs_ctx;
class Fastqs
{
public:
    using uint32 = unsigned int;
    Fastqs(const FastqIOParams& ps = FastqIOParams());
    ~Fastqs();
    static void RegAll();

    static void Gz2Hmz(CString in, CString out, const FastqIOParams& = FastqIOParams());
    static void CompressFile(CString in, CString out);
    static bool readOneRead(CString in, Read* r);
    static bool compressGz2Qp(CString in, CString out);

    void gz2Hmz(CString in, CString out);

private:
    _Fastqs_ctx* m_ptr {nullptr};
};

}
