#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include "c_common.h"
#include "common.h"
#include "_utils.h"

namespace h7 {

#define __BBO_PUT0(name, type)\
        void put##name(type val){\
            prepareBufferIncIfNeed(sizeof (type));\
            memcpy(m_buffer.data() + m_pos, &val, sizeof (type));\
            m_pos += sizeof (type);\
        }

#define __BIO_GET0(name, type)\
    type get##name(){\
        type val;\
        memcpy(&val, m_buf->data() + m_pos, sizeof (type));\
        m_pos += sizeof (type);\
        return val;\
    }

    class ByteBufferOut{
    public:
        ByteBufferOut(uint64 size){
            m_buffer.resize(size);
        }
        ByteBufferOut(){}
        __BBO_PUT0(Byte, char);
        __BBO_PUT0(Int, int);
        __BBO_PUT0(Short, short);
        __BBO_PUT0(Long, sint64);
        __BBO_PUT0(Bool, bool);
        __BBO_PUT0(Float, float);
        __BBO_PUT0(Double, double);

        __BBO_PUT0(UByte, unsigned char);
        __BBO_PUT0(UShort, unsigned short);
        __BBO_PUT0(UInt, unsigned int);
        __BBO_PUT0(ULong, uint64);

        void putData(const void* data, uint64 len){
            prepareBufferIncIfNeed(len);
            memcpy(m_buffer.data() + m_pos, data, len);
            m_pos += len;
        }
        void putRawString(CString str){
            putData(str.data(), str.length());
        }
        void putString16(CString str){
            prepareBufferIncIfNeed(sizeof (uint16) + str.length());
            putUShort(str.length());
            if(str.length() > 0){
                putData(str.data(), str.length());
            }
        }

        void putString(CString str){
            prepareBufferIncIfNeed(sizeof (uint32) + str.length());
            putUInt(str.length());
            if(str.length() > 0){
                putData(str.data(), str.length());
            }
        }
        void putString64(CString str){
            prepareBufferIncIfNeed(sizeof (uint64) + str.length());
            putULong(str.length());
            if(str.length() > 0){
                putData(str.data(), str.length());
            }
        }
        //binStr only contains: '01010010101001010'
        void putBinaryStringAsInts(CString binStr){
            String str = binStr;
            auto str_len = binStr.length();
            //encode binary to oct
            std::vector<int> vec_str;
            {
                int left = 32 - str_len % 32;
                if(left != 32){
                    str.reserve(str_len + left);
                    for(int i = 0 ; i < left ; ++i){
                        str += "0";
                    }
                }
                int size = str_len >> 5; // x/32
                auto _data = str.data();
                vec_str.resize(size);
                for(int i = 0 ; i < size ; ++i){
                    vec_str[i] = h7::binaryStr2Dec(_data + (i << 5), 32);
                }
            }
            uint64 data_size = vec_str.size() * sizeof (int);
            prepareBufferIncIfNeed(sizeof (uint32) * 2 + data_size);
            //str.len, int count, data.
            putULong(str_len);
            putUInt(vec_str.size());
            putData(vec_str.data(), data_size);
        }
        void putListInt(const std::vector<int>& vec){
            prepareBufferIncIfNeed(vec.size() * sizeof (int));
            uint64 data_size = vec.size() * sizeof (int);
            putUInt(vec.size());
            putData(vec.data(), data_size);
        }
        void putMap(const std::unordered_map<char, int>& out){
            prepareBufferIncIfNeed(64);
            putInt(out.size());
            //
            auto it = out.begin();
            for(; it != out.end(); ++it){
                putByte(it->first);
                putInt(it->second);
            }
        }
        //------------------------
        uint64 getLength(){
            return m_pos;
        }
        void bufferToString(String& out){
            out.append(m_buffer.data(), m_pos);
        }
        String bufferToString(){
            String str(m_buffer.data(), m_pos);
            return str;
        }
        void prepareBuffer(uint64 len){
            m_buffer.resize(len);
        }
        void prepareBufferInc(uint64 len){
            m_buffer.resize(m_buffer.size() + len);
        }
        void prepareBufferIncIfNeed(uint64 len){
            auto total = m_buffer.size();
            if(m_pos + len > total){
                if(m_autoInc){
                    prepareBufferInc(m_pos + len - total);
                }else{
                    ASSERT(false, "prepareBufferIncIfNeed failed for autoInc = false.");
                }
            }
        }
        void setBufferAutoInc(bool _auto){
            m_autoInc = _auto;
        }
    private:
        std::vector<char> m_buffer;
        uint64 m_pos{0};
        bool m_autoInc {true};
    };

    class ByteBufferIO{
    public:
        ByteBufferIO(String* str): m_buf(str), m_needFree(false){
        }
        ByteBufferIO(CString str): m_needFree(true){
            m_buf = new String();
            m_buf->resize(str.length());
            *m_buf = str;
        }
        ~ByteBufferIO(){
            if(m_needFree && m_buf){
                delete m_buf;
                m_buf = nullptr;
            }
        }
        void append(CString str){
            m_buf->append(str);
        }
        void append(const void* data, size_t len){
            m_buf->append((char*)data, len);
        }
        void trim(){
            if(m_pos > 0){
                uint64 len_left = m_buf->length() - m_pos;
                memmove(m_buf->data(), m_buf->data() + m_pos, len_left);
                m_pos = 0;
            }
        }
        void skip(size_t len){
            m_pos += len;
        }
        __BIO_GET0(Byte, char);
        __BIO_GET0(Int, int);
        __BIO_GET0(Short, short);
        __BIO_GET0(Long, sint64);
        __BIO_GET0(Bool, bool);
        __BIO_GET0(Float, float);
        __BIO_GET0(Double, double);

        __BIO_GET0(UByte, unsigned char);
        __BIO_GET0(UShort, unsigned short);
        __BIO_GET0(UInt, unsigned int);
        __BIO_GET0(ULong, uint64);

        void getData(uint64 len, void* out){
            memcpy(out, getCurDataPtr(), len);
            m_pos += len;
        }
        String getRawString(uint64 len){
            String str;
            str.resize(len);
            getData(len, str.data());
            return str;
        }
        String getString16(){
            auto len = getUShort();
            String str;
            str.resize(len);
            getData(len, str.data());
            return str;
        }
        String getString(){
            auto len = getUInt();
            String str;
            str.resize(len);
            getData(len, str.data());
            return str;
        }
        String getString64(){
            auto len = getULong();
            String str;
            str.resize(len);
            getData(len, str.data());
            return str;
        }

        String getBinaryStringFromInts(){
            String ret;
            std::vector<int> vec;
            auto str_len = getULong();
            auto size = getUInt();
            uint64 data_size = size * sizeof (int);
            vec.resize(size);
            getData(data_size, vec.data());

            ret.reserve(data_size * 2);
            for(int i = 0 ; i < size ; ++i){
                ret += decToBinaryString(vec[i], true);
            }
            //handle last int.
            int left = 32 - str_len % 32;
            if(left != 32){
                ret.resize(ret.length() - left);
            }
            return ret;
        }
        void getListInt(std::vector<int>& out){
            auto size = getInt();
            for(int i = 0 ; i < size ; ++i){
                out.push_back(getInt());
            }
        }
        void getMap(std::unordered_map<char, int>& out){
            auto size = getInt();
            for(int i = 0 ; i < size ; ++i){
                char key = getByte();
                int val = getInt();
                out[key] = val;
            }
        }

        //--------------------
        uint64 getLeftLength(){
            return m_buf->length() - m_pos;
        }
        uint64 getLength(){
            return m_buf->length();
        }

    private:
        char* getCurDataPtr(){
            return m_buf->data() + m_pos;
        }
    private:
        String* m_buf;
        uint64 m_pos{0};
        bool m_needFree;
    };
}
