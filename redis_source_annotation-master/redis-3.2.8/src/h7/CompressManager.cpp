#include "CompressManager.h"
#include "common.h"
#include "ZstdUtil.h"

using namespace h7;


struct ZSTD_CM: public Compressor{

    uint64 compress(CString in, String* out) override{
        ZstdUtil::StreamCompressString(in, *out, ZSTD_defaultCLevel());
        return out->length();
    }
    uint64 deCompress(CString in, String* out) override{
        if(ZstdUtil::StreamDecompressString(in, *out) == 0){
            return out->length();
        }
        return 0;
    }
    uint64 compress(const void* data, uint64 len, String* out) override{
        return compress(String((char*)data, len), out);
    }
    uint64 deCompress(const void* data, uint64 len, String* out) override{
        return deCompress(String((char*)data, len), out);
    }
};

CompressManager* CompressManager::get(){
    static CompressManager cm;
    return &cm;
}
void CompressManager::regAll(){
    if(get()->m_map.size() > 0){
        return;
    }
    get()->reg(kCompress_ZSTD, std::make_shared<ZSTD_CM>());
}
