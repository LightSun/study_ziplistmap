#include "ZipMap.h"
#include "h7_redis.hpp"
#include "ZipList.h"

using namespace h7;

#define LOOP_zipmap(code)\
    unsigned char *i = zipmapRewind(m_ptr);\
    unsigned char *key, *value;\
    unsigned int klen, vlen;\
    while((i = zipmapNext(i,&key,&klen,&value,&vlen)) != NULL) {\
        code;\
    }


ZipMap::ZipMap(){
    m_ptr = zipmapNew();
}
ZipMap::~ZipMap(){
    if(m_ptr){
        zfree(m_ptr);
        m_ptr = nullptr;
    }
}
void ZipMap::put(CString key, CString val){
    m_ptr = zipmapSet(m_ptr, (u8*)key.data(), key.length(),
              (u8*)val.data(), val.length(), NULL);
}
ZipMap::String ZipMap::get(CString key, CString def){
    u8* val;
    unsigned int val_len;
    if(zipmapGet(m_ptr, (u8*)key.data(), key.length(), &val, &val_len)){
        return String((char*)val, val_len);
    }
    return def;
}
bool ZipMap::containsKey(CString key){
    return zipmapExists(m_ptr, (u8*)key.data(), key.length());
}
bool ZipMap::containsValue(CString val){
    unsigned char *i = zipmapRewind(m_ptr);
    unsigned char *key, *value;
    unsigned int klen, vlen;

    while((i = zipmapNext(i,&key,&klen,&value,&vlen)) != NULL) {
        //printf("  %d:%.*s => %d:%.*s\n", klen, klen, key, vlen, vlen, value);
        if(String((char*)value, vlen) == val){
            return true;
        }
    }
    return false;
}
int ZipMap::size(){
    return zipmapLen(m_ptr);
}
void ZipMap::clear(){
    zfree(m_ptr);
    m_ptr = zipmapNew();
}
//----------------------------
void ZipMap::keys(ZipList* list){
    LOOP_zipmap({
        list->add(String((char*)key, klen));
                })
}
std::vector<ZipMap::String> ZipMap::keys(){
    std::vector<ZipMap::String> vec;
    LOOP_zipmap({
        vec.emplace_back((char*)key, klen);
                });
    return vec;
}
void ZipMap::values(ZipList* list){
    LOOP_zipmap({
        list->add(String((char*)value, vlen));
                })
}
std::vector<ZipMap::String> ZipMap::values(){
    std::vector<ZipMap::String> vec;
    LOOP_zipmap({
        vec.emplace_back((char*)value, vlen);
                });
    return vec;
}
std::map<ZipMap::String, ZipMap::String> ZipMap::toMap(){
    std::map<ZipMap::String, ZipMap::String> map;
    LOOP_zipmap({
       map[String((char*)key, klen)] = String((char*)value, vlen);
                });
    return map;
}
