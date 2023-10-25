#include "DNACompressManager.h"
#include "Huffman.h"

using namespace h7;

struct HuffmanCompressor : public Compressor{

    HuffmanCompressor(){

    }
    ~HuffmanCompressor(){};
    uint64 compress(CString in, String* out) override{
        //avoid memory copy
        Huffman huffman;
        *out = huffman.compress(in);
        return out->length();
    }
    uint64 deCompress(CString in, String* out) override{
        Huffman huffman;
        *out = huffman.decompress(in);
        return out->length();
    }
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
    if(get()->m_map.size() > 0){
        return;
    }
    get()->reg(kDNA_COMPRESS_HUFFMAN, std::make_shared<HuffmanCompressor>());
}

