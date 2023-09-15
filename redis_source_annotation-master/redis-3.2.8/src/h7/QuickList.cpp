#include "QuickList.h"
#include "h7_redis.hpp"

using namespace h7;

QuickList::QuickList(){
    m_ptr = quicklistNew(m_max_entry_ev, m_compress);
}
QuickList::~QuickList(){
    if(m_ptr){
        quicklistRelease(m_ptr);
        m_ptr = nullptr;
    }
}

void QuickList::setParams(int max_entry_ev_list, int compress){
    //max_entry_ev_list: max = 1 << 15. compress >= 0
    quicklistSetFill(m_ptr, max_entry_ev_list);
    quicklistSetCompressDepth(m_ptr, compress);
    m_max_entry_ev = max_entry_ev_list;
    m_compress = compress;
}

void QuickList::add(int index, CString data){
    quicklistEntry entry;
    if(quicklistIndex(m_ptr, index, &entry)){
        quicklistInsertBefore(m_ptr, &entry, (void*)data.data(), data.length());
    }else{
        quicklistPushTail(m_ptr, (void*)data.data(), data.length());
    }
}
void QuickList::add(CString data){
    quicklistPushTail(m_ptr, (void*)data.data(), data.length());
}
void QuickList::addFirst(CString data){
    quicklistPushHead(m_ptr, (void*)data.data(), data.length());
}
QuickList::String QuickList::get(int index){
    quicklistEntry entry;
    if(quicklistIndex(m_ptr, index, &entry)){
        if(entry.value){
            return String((char*)entry.value, entry.sz);
        }
        return std::to_string(entry.longval);
    }
    return "";
}
int QuickList::indexOf(CString data){
    auto it = quicklistGetIterator(m_ptr, AL_START_HEAD);
    quicklistEntry entry;
    int index = 0;
    while (quicklistNext(it, &entry)) {
        if(quicklistCompare(entry.zi, (u8*)data.data(), data.length())){
            goto success;
        }
        index++;
    }
    quicklistReleaseIterator(it);
    return -1;

success:
    quicklistReleaseIterator(it);
    return index;
}
void QuickList::removeAt(int index){
    quicklistEntry entry;
    if(quicklistIndex(m_ptr, index, &entry)){
        auto it = quicklistGetIterator(m_ptr, AL_START_HEAD);
        quicklistDelEntry(it, &entry);
        quicklistReleaseIterator(it);
    }
}
void QuickList::set(int index, CString val){
    quicklistReplaceAtIndex(m_ptr, index, (void*)val.data(), val.length());
}
int QuickList::size(){
    return quicklistCount(m_ptr);
}
void QuickList::clear(){
    quicklistRelease(m_ptr);
    m_ptr = quicklistNew(m_max_entry_ev, m_compress);
}
