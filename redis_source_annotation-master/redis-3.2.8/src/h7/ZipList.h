#pragma once

#include <string>

namespace h7 {

class ZipList
{
public:
    using u8 = unsigned char;
    using String = std::string;
    using CString = const std::string&;

    ZipList();
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

private:
    u8* m_ptr {nullptr};
};

}
