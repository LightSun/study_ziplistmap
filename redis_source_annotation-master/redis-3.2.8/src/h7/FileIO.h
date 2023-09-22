#pragma once

#include "common.h"
#include "c_common.h"
#include <vector>
#include <string.h>
#include <functional>

namespace h7 {

    class FileInput{
    public:
        FileInput(){}
        FileInput(CString path){
            m_file = fopen64(path.data(), "rb");
        }
        ~FileInput(){
            close();
        }
        void open(CString path){
            close();
            m_file = fopen64(path.data(), "rb");
        }
        bool is_open(){
            return m_file != nullptr;
        }
        sint64 getLength(){
            if(m_file == nullptr){
                return 0;
            }
            fseeko64(m_file, 0, SEEK_END);
            return ftello64(m_file);
        }
        void reset(){
            if(m_file != nullptr){
                fseeko64(m_file, 0, SEEK_SET);
            }
        }
        bool seek(uint64 pos, bool end = false){
            if(m_file == nullptr){
                return false;
            }
            if(end){
                fseeko64(m_file, 0, SEEK_END);
            }else{
                fseeko64(m_file, pos, SEEK_SET);
            }
            return true;
        }
        sint64 read(void* data, uint64 len){
            if(m_file == nullptr)
                return -1;
            return fread(data, 1, len, m_file);
        }

        String read(uint64 len){
            ASSERT(len > 0, "");
            if(m_file != nullptr){
                std::vector<char> vec(len, 0);
                fread(vec.data(), 1, len, m_file);
                return String(vec.data(), len);
            }
            return "";
        }
        bool readline(std::string& str) {
            str.clear();
            char ch;
            while (fread(&ch, 1, 1, m_file)) {
                if (ch == '\n') {
                    // unix: LF
                    return true;
                }
                if (ch == '\r') {
                    // dos: CRLF
                    // read LF
                    if (fread(&ch, 1, 1, m_file) && ch != '\n') {
                        // mac: CR
                        fseek(m_file, -1, SEEK_CUR);
                    }
                    return true;
                }
                str += ch;
            }
            return str.length() != 0;
        }
        std::vector<String> readLines(){
            std::vector<String> ret;
            String line;
            while (readline(line)) {
                ret.push_back(line);
            }
            return ret;
        }
        bool readLastLine(String& str){
           bool ret = readline(str);
           while (readline(str)) {
           }
           return ret;
        }
        bool read2Vec(std::vector<char>& buf){
            if(m_file == nullptr){
                return false;
            }
            buf.resize(getLength());
            reset();
            read(buf.data(), buf.size());
            return true;
        }
        bool read2Str(String& out){
            std::vector<char> buf;
            if(!read2Vec(buf)){
                return false;
            }
            out = String(buf.data(), buf.size());
            return true;
        }
        void close(){
            if(m_file != nullptr){
                fclose(m_file);
                m_file = nullptr;
            }
        }
    private:
        FILE* m_file{nullptr};
    };

    class FileOutput{
    public:
        FileOutput(){}
        FileOutput(CString path){
            m_file = fopen(path.data(), "wb");
        }
        ~FileOutput(){
            close();
        }
        void open(CString path){
            close();
            m_file = fopen(path.data(), "wb");
        }
        bool is_open(){
            return m_file != nullptr;
        }
        void close(){
            if(m_file != nullptr){
                fclose(m_file);
                m_file = nullptr;
            }
        }
        bool writeLine(CString line){
            if(write(line.data(), line.size())){
                newLine();
                return true;
            }
            return false;
        }
        bool write(const void* data, size_t size){
            if(m_file == nullptr){
                return false;
            }
            if(fwrite(data, 1, size, m_file) < size){
                //no left space.
                return false;
            }
            return true;
        }
        void newLine(){
            if(m_file != nullptr){
                String new_line = CMD_LINE;
                fwrite(new_line.data(), 1, new_line.length(), m_file);
            }
        }
        void flush(){
            if(m_file != nullptr){
                fflush(m_file);
            }
        }
    private:
        FILE* m_file{nullptr};
    };

    class BufferFileOutput{
    public:
        using Func_Buffer = std::function<bool(char* data, uint64 size)>;
        BufferFileOutput(uint64 fresh_threshold):m_fresh_thd(fresh_threshold){
        }
        void open(CString path){
            close();
            m_file = fopen64(path.data(), "wb");
        }
        bool is_open(){
            return m_file != nullptr;
        }
        void close(){
            if(m_file != nullptr){
                fclose(m_file);
                m_file = nullptr;
            }
        }
        void seek(uint64 pos){
            if(m_file){
                fseeko64(m_file, pos, SEEK_SET);
            }
        }
        bool writeLine(CString line,Func_Buffer func, uint32* rc){
            if(write(line.data(),line.size(), func, rc)){
                newLine();
                return true;
            }
            return false;
        }
        bool write(CString data, Func_Buffer func, uint32* rc){
            return write(data.data(), data.length(), func, rc);
        }
        bool write(const void* data, size_t size,
                   Func_Buffer func, uint32* rc){
            if(size == 0){
                return false;
            }
            if(m_data.size() + size > m_fresh_thd){
                if(!flush(func, rc)){
                    return false;
                }
            }
            uint32 old_pos = m_data.size();
            m_data.resize(old_pos + size);
            memcpy(m_data.data() + old_pos, data, size);
            m_writeCount ++;
            return true;
        }
        void newLine(){
            String new_line = CMD_LINE;
            Func_Buffer func;
            write(new_line.data(), new_line.length(), func, nullptr);
        }
        bool fWriteLine(CString line){
            if(fWrite(line.data(), line.size())){
                fNewLine();
                return true;
            }
            return false;
        }
        bool fWrite(CString data){
            return fWrite(data.data(), data.length());
        }
        bool fWrite(const void* data, size_t size){
            if(m_file == nullptr){
                return false;
            }
            if(std::fwrite(data, 1, size, m_file) < size){
                //no left space.
                return false;
            }
            return true;
        }
        void fNewLine(){
            if(m_file != nullptr){
                String new_line = CMD_LINE;
                std::fwrite(new_line.data(), 1, new_line.length(), m_file);
            }
        }
        void fFlush(){
            if(m_file != nullptr){
                fflush(m_file);
            }
        }
        bool flush(Func_Buffer func, uint32* rc){
            if(m_data.size() == 0){
                return true;
            }
            bool ret;
            if(func){
                ret = func(m_data.data(), m_data.size());
            }else{
                ret = fWrite(m_data.data(), m_data.size());
            }
            if(!ret){
                return false;
            }
            m_data.clear();
            if(rc){
                *rc = m_writeCount;
            }
            m_writeCount = 0;
            if(m_file != nullptr){
                fflush(m_file);
            }
            return true;
        }
    private:
        uint64 m_fresh_thd;
        FILE* m_file{nullptr};
        uint32 m_writeCount {0};
        std::vector<char> m_data;
    };
}
