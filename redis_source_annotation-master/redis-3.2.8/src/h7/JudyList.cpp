#include "JudyList.h"
#include <memory.h>
#include <vector>
#include <sstream>
#include <string.h>

#include "common.h"

#define JUDY_INTERNAL

#ifdef JUDY_INTERNAL
#include "judy_arrays.h"
#else
#include <Judy.h>
#endif
//JSLI not permit dup key.
using namespace h7;
namespace h7 {
    using String = JudyList::String;
    using CString = JudyList::CString;

#ifdef JUDY_INTERNAL
    struct _JudyList_ctx{
        Judy m_judy {nullptr};
        int m_size {0};
        uint32_t m_max_entry;

        _JudyList_ctx(uint32_t max_entry):m_max_entry(max_entry){
            m_judy = judy_open(max_entry);
        }
        ~_JudyList_ctx(){
            if(m_judy){
                judy_close(m_judy);
                m_judy = nullptr;
            }
        }
        judyslot _newSlot(CString c){
            void* v = judy_data(m_judy, c.length() + 1);
            memcpy(v, c.data(), c.length() + 1);
            return (judyslot)v;
        }
        int size(){
            return m_size;
        }
        void add(CString c){
            add(size(), c);
        }
        void add(int index, CString c){
            auto slot = judy_cell(m_judy, (uchar*)&index, sizeof (int));
            if(*slot){
                judyslot preVal = *slot;
                //*slot = _newSlot(c); //error?
                while (true) {
                    auto slot2 = judy_next(m_judy);
                    if(!slot2){
                        break;
                    }
                    judyslot _val = *slot2;
                    *slot2 = preVal;
                    preVal = _val;
                }
                //add last
                slot = judy_cell(m_judy, (uchar*)&m_size, sizeof (int));
                *slot = preVal;
                //
                slot = judy_slot(m_judy, (uchar*)&index, sizeof (int));
                *slot = _newSlot(c);
            }else{
                *slot = _newSlot(c);
            }
            m_size ++;
        }
        void set(int index, CString c){
            auto slot = judy_slot(m_judy, (uchar*)&index, sizeof (int));
            if(*slot){
                judy_del(m_judy);
            }
            slot = judy_cell(m_judy, (uchar*)&index, sizeof (int));
            *slot = _newSlot(c);
        }
        String get(int index, CString def){
            auto slot = judy_slot(m_judy, (uchar*)&index, sizeof (int));
            if(*slot){
                return String((char*)(*slot));
            }
            return def;
        }
        void removeAt(int index){
            auto slot = judy_slot(m_judy, (uchar*)&index, sizeof (int));
            if(*slot){
                judy_del(m_judy);
            }
           // int last = m_size - 1;
           // String lastVal = get(last, "");
           for(int i = index, nextIdx; i < m_size - 1; ++i){
                nextIdx = i + 1;
                slot = judy_cell(m_judy, (uchar*)&i, sizeof (int));

                auto slot2 = judy_slot(m_judy, (uchar*)&nextIdx, sizeof (int));
                *slot = *slot2;
                if(nextIdx == m_size - 1){
                    *slot2 = 0;
                    judy_del(m_judy);
                }
            }
            m_size --;
        }
        void clear(){
            if(m_judy){
                judy_close(m_judy);
            }
            m_judy = judy_open(m_max_entry);
        }
        int indexOf(CString c){
            auto slot = judy_start(m_judy, NULL, 0);
            if(*slot){
                int index = 0;
                while (judy_key(m_judy, (uchar*)&index, sizeof(int)) > 0) {
                    if(memcmp(c.data(), (void*)(*slot), c.length()) == 0){
                        return index;
                    }
                    slot = judy_next(m_judy);
                }
            }
            return -1;
        }
        void print(){
            std::stringstream ss;
            ss << "[";
            auto slot = judy_start(m_judy, NULL, 0);
            if(*slot){
                int index = 0;
                while (judy_key(m_judy, (uchar*)&index, sizeof(int)) > 0) {
                    ss << (char*)(*slot);
                    slot = judy_next(m_judy);
                    if(slot && *slot){
                        ss << ",";
                    }
                }
            }
            ss << "]";
            auto str = ss.str();
            printf("judy_print >>\n %s\n", str.data());
        }
        void toVector(std::vector<String>& vec){
            auto slot = judy_start(m_judy, NULL, 0);
            if(*slot){
                int index = 0;
                while (judy_key(m_judy, (uchar*)&index, sizeof(int)) > 0) {
                    vec.emplace_back((char*)(*slot));
                    slot = judy_next(m_judy);
                }
            }
        }
    };
#else
    static inline void* _newChars(CString c){
        void* ptr = malloc(c.length() + 1);
        memcpy(ptr, c.data(), c.length() + 1);
        return ptr;
    }
    struct _JudyList_ctx{
        Pvoid_t PJArray {nullptr};
        int _size {0};

        _JudyList_ctx(uint32_t max){
            //
        }

        int size(){
            return _size;
        }
        void set(int index, CString c){
            set(index, _newChars(c));
        }
        void set(int index, void* p){
            JError_t err;
            auto ret = JudyLGet(PJArray, index, &err);
            if(ret != PJERR){
                PWord_t PValue = (PWord_t)ret;
                *PValue = (Word_t)p;
            }
        }
        void add(JudyList::CString c){
            _add(size(), _newChars(c));
        }
        void _add(Word_t index ,CString c){
            _add(index, _newChars(c));
        }
        void _add(Word_t id, void* p){
            //JSLI(val, arr, data);
            //JLI;
            JError_t err;
            auto ret = JudyLIns(&PJArray, id, &err);
            if(ret != PJERR){
                PWord_t PValue = (PWord_t)ret;
                *PValue = (Word_t)p;
            }else{
                J_E("JudySLIns", &err);
            }
            _size ++;
        }
        void add(Word_t index, CString c){
            //JLG;
            const int old_size = _size;
            JError_t err;
            std::vector<Word_t> posts;
            std::vector<Word_t> indexes;
            {
                Word_t idx = index;
                PPvoid_t ret = JudyLGet(PJArray, idx, &err);
                while (ret != PJERR && ret != nullptr){
                    PWord_t PValue = (PWord_t)ret;
                    posts.push_back(*PValue);
                    indexes.push_back(idx + 1);
                    ret = JudyLNext(PJArray, &idx, &err);
                }
            }
            _add(index, c);
            if(posts.size() == 0){
                return;
            }
//            int rc = JudyLInsArray(&PJArray, posts.size(),
//                                   indexes.data(),
//                                   posts.data(), &err);
//            if(rc != 1){
//                J_SE("JudyLInsArray", (JU_Errno_t)rc);
//            }
            {
                int size = posts.size();
                for(int i = 0 ; i < size ; ++i){
                    _add(indexes[i], (void*)posts[i]);
                }
            }
            _size = old_size + 1;
        }
        JudyList::String get(int index, CString def){
            JError_t err;
            auto ret = JudyLGet((Pvoid_t)PJArray, index, &err);
            if(ret != PJERR){
                return JudyList::String(*((char**)ret));
            }else{
                return def;
            }
        }
        void removeAt(int index){
            const int old_size = _size;
            JError_t err;
            PPvoid_t ret;
            std::vector<Word_t> posts;
            std::vector<Word_t> indexes;
            {
                Word_t idx = index;
                ret = JudyLGet(PJArray, idx, &err);
                if(ret == PJERR || ret == NULL){
                    return;
                }
                {
                    PWord_t PValue = (PWord_t)ret;
                    free(*(char**)PValue);
                    JudyLDel(&PJArray, idx, &err);
                }
                ret = JudyLNext(PJArray, &idx, &err);
                while (ret != PJERR && ret != NULL){
                    PWord_t PValue = (PWord_t)ret;
                    posts.push_back(*PValue);
                    indexes.push_back(idx - 1);
                    JudyLDel(&PJArray, idx, &err);
                    ret = JudyLNext(PJArray, &idx, &err);
                }
            }
            if(posts.size() == 0){
                _size = old_size - 1;
                return;
            }
            {
                int size = posts.size();
                for(int i = 0 ; i < size ; ++i){
                    _add(indexes[i], (void*)posts[i]);
                }
            }
            _size = old_size - 1;
        }
        void clear(){
            JError_t err;
            JudyLFreeArray(&PJArray, &err);
            _size = 0;
            PJArray = nullptr;
        }
        int indexOf(CString c){
            PWord_t PValue;
            Word_t index = 0;
            JError_t err;
            PValue = (PWord_t)JudyLFirst(PJArray , &index, &err);
            if(PValue == PJERR){
                return -1;
            }
            while (PValue != NULL && PValue != PJERR) {
                //JudyList::String str = *(char**)PValue;
                //printf("str = %s\n", str.data());
                if(strcmp(c.data(), *(char**)PValue) == 0){
                    return index;
                }
                PValue = (PWord_t)JudyLNext(PJArray , &index, &err);
            }
            return -1;
        }
        void print(){
            PWord_t PValue;
            Word_t index = 0;
            JError_t err;
            PValue = (PWord_t)JudyLFirst(PJArray , &index, &err);
            if(PValue == PJERR){
                return;
            }
            std::stringstream ss;
            while (PValue != NULL) {
                ss << *(char**)PValue;
                PValue = (PWord_t)JudyLNext(PJArray , &index, &err);
                if(PValue == PJERR || PValue == NULL){
                    break;
                }
                ss << ", ";
            }
            auto str = ss.str();
            printf("judy_print >>\n [%s]\n", str.data());
        }
        void toVector(std::vector<String>& vec){
            PWord_t PValue;
            Word_t index = 0;
            JError_t err;
            PValue = (PWord_t)JudyLFirst(PJArray , &index, &err);
            while (PValue != NULL && PValue != PJERR) {
                vec.emplace_back(*(char**)PValue);
                PValue = (PWord_t)JudyLNext(PJArray , &index, &err);
            }
        }
    };
#endif
}

JudyList::JudyList(uint32 max_entry){
    m_ptr = new _JudyList_ctx(max_entry);
}
JudyList::~JudyList(){
    if(m_ptr){
        delete m_ptr;
        m_ptr = nullptr;
    }
}
void JudyList::addFirst(CString data){
    m_ptr->add(0, data);
}
void JudyList::add(int index, CString data){
    m_ptr->add(index, data);
}
void JudyList::add(CString data){
    m_ptr->add(data);
}
JudyList::String JudyList::get(int index){
    return m_ptr->get(index, "");
}
void JudyList::set(int index, CString val){
    m_ptr->set(index, val);
}
int JudyList::size(){
    return m_ptr->size();
}

int JudyList::indexOf(CString data){
    return m_ptr->indexOf(data);
}
void JudyList::removeAt(int index){
    m_ptr->removeAt(index);
}
void JudyList::clear(){
    m_ptr->clear();
}
void JudyList::print(){
    m_ptr->print();
}
void JudyList::toVector(std::vector<String>& vec){
    m_ptr->toVector(vec);
}
