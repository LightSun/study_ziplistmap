#include "CompressManager.h"
#include "DNACompressManager.h"

using namespace h7;

static void test_common();
static void test_dna_compress();
static void test_dna_compress2();

void test_compressor(){
    test_common();
    test_dna_compress();
    test_dna_compress2();
}

void test_common(){
    printf("---------- test_common -------- \n");
    CompressManager::get()->regAll();
    auto cm = CompressManager::get()->getCompressor(kCompress_ZSTD);
    String str = "fdgjkdfkjg899erjterkgeu89^&%&^%&^%&S$#!~@#$%&*()_)+fkgdgdf87g8df98gd8fjtkjweljkgsdpp[m,lv,m";
    String out;
    ASSERT(cm->compress(str, &out) > 0, "compress failed");
    String str2;
    ASSERT(cm->deCompress(out, &str2) > 0, "deCompress failed");

    ASSERT(str == str2, "decode ok, but not eq with previous!");
}

void test_dna_compress(){
    printf("---------- test_dna_compress -------- \n");
    DNACompressManager::get()->regAll();
    auto cm = DNACompressManager::get()->getCompressor(kDNA_COMPRESS_HUFFMAN);

    String str = "ACTGTTTCCCGGGAAACTATATATCTCTGCGCGTGTTTG";
    String out;
    ASSERT(cm->compress(str, &out) > 0, "compress failed");
    String str2;
    ASSERT(cm->deCompress(out, &str2) > 0, "deCompress failed");

    ASSERT(str == str2, "decode ok, but not eq with previous!");
}
void test_dna_compress2(){
    printf("---------- test_dna_compress2 -------- \n");
    DNACompressManager::get()->regAll();
    auto cm = DNACompressManager::get()->getCompressor(kDNA_COMPRESS_HUFFMAN);

    String str = "FFKKKFKKFKF<KK<F,AFKKKKK7FFK77<FKK,<F7K,,7AF<FF7FKK7AA,7<FA,,";
    String out;
    ASSERT(cm->compress(str, &out) > 0, "compress failed");
    String str2;
    ASSERT(cm->deCompress(out, &str2) > 0, "deCompress failed");

    ASSERT(str == str2, "decode ok, but not eq with previous!");
    LOGI("str.len = %lu, compressed len = %lu\n", str.length(), out.length());
}
