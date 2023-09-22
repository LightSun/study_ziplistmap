#include "JudyList.h"
#include <Judy.h>
#include <memory.h>
#include <vector>
#include <sstream>

//JSLI not permit dup key.
using namespace h7;
namespace h7 {

static inline void* _newChars(JudyList::CString c){
    void* ptr = malloc(c.length() + 1);
    memcpy(ptr, c.data(), c.length() + 1);
    return ptr;
}
    struct _Item{
        int index;
        JudyList::String data;
    };
    struct _JudyList_ctx{
        Pvoid_t PJArray {nullptr};
        int _size {0};

        int size(){
            return _size;
        }
        void set(int index, JudyList::CString c){
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
        void _add(Word_t index ,JudyList::CString c){
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
        void add(Word_t index, JudyList::CString c){
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
        JudyList::String get(int index, JudyList::CString def){
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
        int indexOf(JudyList::CString c){
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
        void toVector(std::vector<JudyList::String>& vec){
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
}

JudyList::JudyList(){
    m_ptr = new _JudyList_ctx();
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
