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
    //void fromNonSeqString(CString str);
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
    ReadWriter_HMZ(CString file, unsigned int max_read_count = 100000);
    ~ReadWriter_HMZ();
    void setParams(const Params&) override;
    void begin() override;
    bool write(Read* r) override;
    void end() override;

private:
    _ReadWriter_HMZ_ctx* m_ptr{nullptr};
};

typedef struct _ReadReader_HMZ_ctx __ReadReader_HMZ_ctx_ctx;
class ReadReader_HMZ: public ReadReader{
public:
    ReadReader_HMZ(CString file);
    ~ReadReader_HMZ();
    bool nextRead(Read* r) override;

private:
    _ReadReader_HMZ_ctx* m_ptr {nullptr};
};

//-------------------------------------
class Fastqs
{
public:
    Fastqs();
};

}
