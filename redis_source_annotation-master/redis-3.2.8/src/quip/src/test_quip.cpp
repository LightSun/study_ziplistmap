#include <string>
#include <vector>
#include "quip.h"
#include "Fastq_fastp.h"
extern "C"{
    FILE* open_fin(const char* fn);
}

#define _LOG(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)

using String = std::string;
typedef int (*Func_main)(int argc, char* argv[]);

static void test_read0();
static void test_read2();
static void test_for_fastp();

/**
./quip -r /media/heaven7/Elements/shengxin/refHg19/GATK_bundle/hs37d5.fa
/media/heaven7/Elements/shengxin/test/2022EQA/202212/202212proband.R1.fastq.gz
/media/heaven7/Elements/shengxin/test/2022EQA/202212/202212proband.R2.fastq.gz
*/
extern "C" int quip_run_default(const char* exe,Func_main main){
    //test_read0();
    //test_read2();
    test_for_fastp();
    return 0;
    std::vector<String> vec;
    vec.push_back(exe);
    vec.push_back("-r");
    vec.push_back("/media/heaven7/Elements/shengxin/refHg19/GATK_bundle/hs37d5.fa");
    vec.push_back("/media/heaven7/Elements/shengxin/test/2022EQA/"
                  "202212/202212proband.R1.fastq.gz");
    vec.push_back("/media/heaven7/Elements/shengxin/test/2022EQA/"
                  "202212/202212proband.R2.fastq.gz");

    std::vector<const char*> ptrs;
    for(size_t i = 0 ; i < vec.size() ; ++i){
        ptrs.push_back(vec[i].data());
    }
    return main(vec.size(), (char**)ptrs.data());
}
//like name = A00869:44:HLYFLDSXX:2:1101:3549:1000,comment=2:N:0:ATGCCTAA+AACCGTTC
/*
id = A00869:44:HLYFLDSXX:2:1101:3549:1000 2:N:0:ATGCCTAA+AACCGTTC
seqname = (null)
seq = NGTGGGAAAGGGGCGCGACATCAGCCTAGCGGCCCTGCAGCGCCACGACCCCTATATCAACCGCATCGTGGACGTGGCCAGCCAGGTGGCTCTGTACACCTTCGGCCATCGGGCCAACGAGTGGGTGCGTGCGGACGCGGCGGAGCAGTG
qual = #F:FFFFFFFFFFFFFFFFFFFFFFFFFF:FFFFFFFFFFFFFFFFFFFF:FF:FF:FFFFFFFFFFFFFFF:FFF:FFFFFFFF:FF:FF::FFFFFFF,FFFFFFFFFFFFFF:FFFFFFFFFFFF:FFFFFFFFFFFFFFFFFFFFF
*/
void test_read0(){
    String inFile = "/media/heaven7/Elements/shengxin/test/2022EQA/202212/"
                    "202212proband.R2.fastq.gz";
    auto fin = open_fin(inFile.data());
    auto in = quip_in_open_file(fin, QUIP_FMT_FASTQ, QUIP_FILTER_GZIP, 0, NULL);
    auto read = quip_read(in);
    //String id((char*)read->id.s, read->id.n);
    _LOG("id = %s\n", read->id.s);
    _LOG("seqname = %s\n", read->seqname.s);
    _LOG("seq = %s\n", read->seq.s);
    _LOG("qual = %s\n", read->qual.s);
    quip_in_close(in);
    fclose(fin);
}
void test_read2(){
    String inFile = "/media/heaven7/Elements/shengxin/test/2022EQA/202212/"
                    "R2.fastq.qp";
    auto fin = open_fin(inFile.data());
    auto in = quip_in_open_file(fin, QUIP_FMT_QUIP, QUIP_FILTER_NONE, 0, NULL);
    auto read = quip_read(in);
    //String id((char*)read->id.s, read->id.n);
    _LOG("id = %s\n", read->id.s);
    _LOG("seqname = %s\n", read->seqname.s);
    _LOG("seq = %s\n", read->seq.s);
    _LOG("qual = %s\n", read->qual.s);
    quip_in_close(in);
    fclose(fin);
}
struct Read{
    String name;
    String seq;
    String strand;
    String qua;
};
void test_for_fastp(){
    String inFile = "/media/heaven7/Elements/shengxin/test/2022EQA/202212/"
                    "R2.fastq.qp";
    using namespace fastp;
    Fastq_fastp fq(inFile);
    Read r;
    int rc = 0;
    while (fq.hasNextRead()) {
        fq.read(&r.name, &r.seq, &r.strand, &r.qua);
        rc ++;
    }
    _LOG("test_for_fastp >. rc = %d\n", rc);
}
