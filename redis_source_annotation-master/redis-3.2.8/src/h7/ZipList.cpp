#include "ZipList.h"
#include "h7_redis.hpp"

using namespace h7;

ZipList::ZipList()
{
    m_ptr = ziplistNew();
}
ZipList::~ZipList(){
    if(m_ptr){
        zfree(m_ptr);
        m_ptr = nullptr;
    }
}

void ZipList::add(int index, CString data){
    if(index > size()){
        return;
    }
    auto p = ziplistIndex(m_ptr, index);
    m_ptr = ziplistInsert(m_ptr, p, (u8*)data.data(), data.length());
}
void ZipList::add(CString data){
    m_ptr = ziplistPush(m_ptr, (u8*)data.data(), data.length(), ZIPLIST_TAIL);
}
void ZipList::addFirst(CString data){
    m_ptr = ziplistPush(m_ptr, (u8*)data.data(), data.length(), ZIPLIST_HEAD);
}
ZipList::String ZipList::get(int index){
    auto p = ziplistIndex(m_ptr, index);
    if(p){
        u8* data;
        unsigned int len;
        long long val;
        if(ziplistGet(p, &data, &len, &val)){
            if(data){
                return String((char*)data, len);
            }else{
                return std::to_string(val);
            }
        }
    }
    return "";
}
int ZipList::indexOf(CString data){
    u8* p = ziplistIndex(m_ptr, 0);
    int index = 0;
    while (p) {
        if(ziplistCompare(p, (u8*)data.data(), data.length())){
            return index;
        }
        index++;
        p = ziplistNext(m_ptr, p);
    }
    return -1;
}
void ZipList::removeAt(int index){
    u8* p = ziplistIndex(m_ptr, index);
    if(p){
        m_ptr = ziplistDelete(m_ptr, &p);
    }
}
void ZipList::set(int index, CString val){
    add(index, val);
    removeAt(index + 1);
    //entry.node->zl = ziplistDelete(entry.node->zl, &entry.zi);
    //entry.node->zl = ziplistInsert(entry.node->zl, entry.zi, data, sz);
}
int ZipList::size(){
    return ziplistLen(m_ptr);
}
void ZipList::clear(){
    zfree(m_ptr);
    m_ptr = ziplistNew();
}
