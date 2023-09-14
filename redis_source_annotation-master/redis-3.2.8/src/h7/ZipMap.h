#pragma once

#include <string>

namespace h7 {

class ZipMap
{
public:
    using u8 = unsigned char;
    using String = std::string;
    using CString = const std::string&;

    ZipMap();
    ~ZipMap();

    void put(CString key, CString val);
    String get(CString key, CString def = "");
    bool containsKey(CString key);
    bool containsValue(CString val);
    int size();
    void clear();

private:
    u8* m_ptr;
};

}

