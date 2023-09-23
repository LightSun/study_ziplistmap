#include "DNACompressManager.h"
#include "Huffman.h"

using namespace h7;

struct HuffmanCompressor : public Compressor{

    HuffmanCompressor(){

    }
    ~HuffmanCompressor(){};
    uint64 compress(const void* data, uint64 len, String* out) override{
        Huffman huffman;
        *out = huffman.compress(String((char*)data, len));
        return out->length();
    }
    uint64 deCompress(const void* data, uint64 len, String* out)override{
        Huffman huffman;
        *out = huffman.decompress(String((char*)data, len));
        return out->length();
    }
};

DNACompressManager* DNACompressManager::get(){
    static DNACompressManager dcm;
    return &dcm;
}

void DNACompressManager::regAll(){
    //
    get()->reg(kDNA_COMPRESS_HUFFMAN, std::make_shared<HuffmanCompressor>());
}

