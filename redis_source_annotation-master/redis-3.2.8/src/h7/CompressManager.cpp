#include "CompressManager.h"
#include "zstd.h"
#include "common.h"

using namespace h7;


struct ZSTD_CM: public Compressor{

    ZSTD_CCtx* cctx {nullptr};
    ZSTD_DCtx* dctx {nullptr};

    ZSTD_CM(){
        cctx = ZSTD_createCCtx();
        auto ret = ZSTD_CCtx_setParameter(cctx, ZSTD_c_strategy, ZSTD_btultra);
        ASSERT(ZSTD_isError(ret), "ZSTD_CCtx_setParameter failed.");

        dctx = ZSTD_createDCtx();
        ASSERT(dctx, "ZSTD_createDCtx failed.");
    }
    ~ZSTD_CM(){
        if(cctx){
            ZSTD_freeCCtx(cctx);
            cctx = nullptr;
        }
        if(dctx){
            ZSTD_freeDCtx(dctx);
            dctx = nullptr;
        }
    }
    uint64 compress(const void* data, uint64 len, String* out) override{
        out->resize(len);
        auto out_size = ZSTD_compressCCtx(cctx, out->data(), len, data,
                                          len, ZSTD_defaultCLevel());
        ASSERT(ZSTD_isError(out_size), "ZSTD_compressCCtx failed.");
        return out_size;
    }
    uint64 deCompress(const void* data, uint64 len, String* out) override{
        out->resize(len);
        auto out_size = ZSTD_decompressDCtx(dctx, out->data(), len, data, len);
        ASSERT(ZSTD_isError(out_size), "ZSTD_decompressDCtx failed.");
        return out_size;
    }
};

CompressManager* CompressManager::get(){
    static CompressManager cm;
    return &cm;
}
void CompressManager::regAll(){
    get()->reg(kCompress_ZSTD, std::make_shared<ZSTD_CM>());
}
