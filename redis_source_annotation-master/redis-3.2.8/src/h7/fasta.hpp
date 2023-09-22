#ifndef FASTA_HPP
#define FASTA_HPP

#include <zlib.h>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>

#include "kseq.h"
#include "common.h"
KSEQ_INIT(gzFile, gzread)

#define GET_SEQ_STR(val) (val.l > 0 ? std::string(val.s,val.l) : "")

namespace hc {
    struct Read{
        std::string name;
        std::string seq;
        std::string qual;
        std::string comment;
        int lastChar;

        size_t maxSize(){
            return name.length() + seq.length()
                    + qual.length() + comment.length()
                    + 1;
        }
    };

    class Fasta{
    public:
        Fasta(const std::string& file, int maxC = 0){
            ASSERT(!file.empty(), "open file failed. file = %s\n", file.data());
            gzFile fp;
            kseq_t *seq;
            //
            FILE* _file = fopen64(file.data(), "rb");
            fp = gzdopen(fileno(_file), "r");
            seq = kseq_init(fp);
           // seq->seq.l 字符串长度. seq->seq.m. 分配的长度. seq->seq.s 字符串
            int readRc = 0;
            while (kseq_read(seq) >= 0){
               struct Read read;
               read.name = GET_SEQ_STR(seq->name);
               read.seq = GET_SEQ_STR(seq->seq);
               read.qual = GET_SEQ_STR(seq->qual);
               read.comment = GET_SEQ_STR(seq->comment);
               read.lastChar = seq->last_char;
               m_map[seq->name.s] = std::move(read);
               m_names.emplace_back(seq->name.s);
               if(maxC > 0){
                   readRc ++;
                   if(readRc >= maxC){
                       break;
                   }
               }
            }
           // printf("%d\t%d\t%d\n", n, slen, qlen);
            kseq_destroy(seq);
            gzclose(fp);
            fclose(_file);
        }
        static std::vector<std::string> readNames(const std::string& file){
            std::vector<std::string> ret;
            gzFile fp;
            kseq_t *seq;
            //
            FILE* _file = fopen64(file.data(), "rb");
            fp = gzdopen(fileno(_file), "r");
            seq = kseq_init(fp);
            int readRc = 0;
            while (kseq_read(seq) >= 0){
               ret.push_back(GET_SEQ_STR(seq->name)) ;
            }
            kseq_destroy(seq);
            gzclose(fp);
            fclose(_file);
            return ret;
        }
        bool checkFq(){
            auto it = m_map.begin();
            for(;it != m_map.end(); it++){
                if(it->second.qual.empty() || it->second.seq.length() != it->second.qual.length()){
                    return false;
                }
            }
            return true;
        }
        //load the seq for target cigar
        std::string_view load(const std::string& cigar) const{
            auto it = m_map.find(cigar);
            if(it != m_map.end()){
                return it->second.seq.data();
            }
            return "";
        }
        std::vector<std::string>& names(){
            return m_names;
        }
        Read& getRead(const std::string& name){
            return m_map[name];
        }

        void printNames()const{
            std::cout << "printNames >> count: " << m_names.size() << std::endl;
            auto it = m_names.begin();
            for(; it != m_names.end(); it ++){
                 std::cout << *it << ", ";
            }
            std::cout << std::endl;
        }
    private:
        std::map<std::string, Read> m_map;
        std::vector<std::string> m_names;
    };
}

#endif // FASTA_HPP
