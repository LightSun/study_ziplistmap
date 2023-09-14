#pragma once

#include <string>

typedef struct quicklist* _quicklistP;

namespace h7 {

class QuickList
{
public:
    using u8 = unsigned char;
    using String = std::string;
    using CString = const std::string&;

    QuickList();
    ~QuickList();
    //compress > 0 means compress .
    //fill
    void setParams(int max_entry_ev_list, int compress);

    void add(int index, CString data);
    void add(CString data);
    void addFirst(CString data);
    String get(int index);
    int indexOf(CString data);
    void removeAt(int index);
    void set(int index, CString val);
    int size();
    void clear();

private:
    _quicklistP m_ptr {nullptr};
    int m_max_entry_ev {32};
    int m_compress {3};
};

}
