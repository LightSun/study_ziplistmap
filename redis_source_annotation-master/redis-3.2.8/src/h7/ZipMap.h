#pragma once

#include <string>
#include <vector>
#include <map>

namespace h7 {

class ZipList;

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

    void keys(ZipList* list);
    std::vector<String> keys();

    void values(ZipList* list);
    std::vector<String> values();

    std::map<String, String> toMap();

private:
    u8* m_ptr;
};

}

