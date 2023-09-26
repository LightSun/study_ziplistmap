#pragma once

#include <string>
#include <vector>

namespace h7 {

typedef struct _JudyList_ctx _JudyList_ctx;

class JudyList
{
public:
    using u8 = unsigned char;
    using uint32 = unsigned int;
    using String = std::string;
    using CString = const std::string&;

    JudyList(uint32 max_entry = 128);
    ~JudyList();

    void add(int index, CString data);
    void add(CString data);
    void addFirst(CString data);
    String get(int index);
    int indexOf(CString data);
    void removeAt(int index);
    void set(int index, CString val);
    int size();
    void clear();

    bool contains(CString data){
        return indexOf(data) >= 0;
    }
    void toVector(std::vector<String>& vec);
    std::vector<String> toVector(){
        std::vector<String> vec;
        toVector(vec);
        return vec;
    }

    void print();
private:
    _JudyList_ctx* m_ptr{nullptr};
};


}
