
#include "Fastqs.h"
#include "common.h"

using namespace h7;

static void test_base();

#define _OUT_DIR String("/home/heaven7/heaven7/work/shengxin/test/compress/")

int test_fq_compress(int argc, char* argv[]){
    //ReadReader_GZ reader();
    Fastqs::RegAll();
    //Fastqs::gz2Hmz(argv[1], argv[2]);

    String inFile = "/media/heaven7/Elements/shengxin/test/2022EQA/202212/"
                    "202212proband.R1.fastq.gz";
    String outFile = _OUT_DIR + "202212proband.R1.fastq.hmz";
    String outFile2 = _OUT_DIR + "202212proband.R1.fastq.hmz2";
    //raw: 13073649953
    Fastqs::Gz2Hmz(inFile, outFile);
    //Fastqs fqs;
    //fqs.gz2Hmz(inFile, outFile);
    //Fastqs::CompressFile(outFile, outFile2);

    //test_base(); //ok
    return 0;
}

void test_base(){
    String file = _OUT_DIR + "test.hmz";
    String str1, str2;
    {
        ReadWriter_HMZ writer(file, 3);
        writer.begin();
        for(int i = 0 ; i < 10 ; i ++){
            auto idstr = std::to_string(i);
            Read r;
            r.name = "dfdsfsdfksd__" + idstr;
            r.seq  = "ACTGTCTGTCGCTCGCTCGCTCGCTCGCTCGACTGCTCGGCTCGGCGCGCGCGGCGCGCGC";
            r.qual = "FFKKKFKKFKF<KK<F,AFKKKKK7FFK77<FKK,<F7K,,7AF<FF7FKK7AA,7<FA,,";
            r.comment = "I am heaven7";
            r.lastChar = ',';
            writer.write(&r);
            str1 += r.toString();
        }
        writer.end();
    }
    {
        ReadReader_HMZ reader(file);
        Read r;
        while (reader.nextRead(&r)) {
            str2 += r.toString();
        }
        ASSERT(str1 == str2, "HMZ >> after write and read, decode ok, but string not EQ!");
    }
}
