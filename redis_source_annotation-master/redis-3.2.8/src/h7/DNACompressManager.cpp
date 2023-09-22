#include "DNACompressManager.h"

using namespace h7;

struct HuffmanCompressor : public Compressor{

    HuffmanCompressor(){

    }
    ~HuffmanCompressor(){};
    uint64 compress(void* data, uint64 len, String* out) override{

    }
    uint64 deCompress(void* data, uint64 len, String* out) override{

    }
};

DNACompressManager* DNACompressManager::get(){
    static DNACompressManager dcm;
    return &dcm;
}

void DNACompressManager::regAll(){
    //
}

