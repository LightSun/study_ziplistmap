#pragma once

#include <string>
#include <vector>
#include <initializer_list>

namespace h7 {

class ZipList
{
public:
    using u8 = unsigned char;
    using String = std::string;
    using CString = const std::string&;

    ZipList();
    ZipList(const std::vector<String>& _list);
    ZipList(const std::initializer_list<String>& _list);
    ZipList(const ZipList& _list);
    ~ZipList();

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

    //---------------------------------------
    void toVector(std::vector<String>& vec);
    std::vector<String> toVector(){
        std::vector<String> vec;
        toVector(vec);
        return vec;
    }

private:
   // ZipList(const ZipList&);
    void operator=(const ZipList&);

    u8* m_ptr {nullptr};
};

}
