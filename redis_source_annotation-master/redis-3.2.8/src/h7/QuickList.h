#pragma once

#include <string>
#include <vector>
#include <initializer_list>

typedef struct quicklist* _quicklistP;

namespace h7 {

class QuickList
{
public:
    using u8 = unsigned char;
    using String = std::string;
    using CString = const std::string&;

    struct InitParams{
        int max_entry_ev_list :32;
        int compress :3;
    };
    using _InitPs = const InitParams&;

    QuickList(_InitPs = InitParams());
    QuickList(const std::vector<String>& _list, _InitPs = InitParams());
    QuickList(const std::initializer_list<String>& _list, _InitPs = InitParams());
    QuickList(const QuickList& list);
    ~QuickList();
    //compress > 0 means compress .
    //fill
    void setParams(_InitPs ip);

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

private:
    //QuickList(const QuickList&);
    void operator=(const QuickList&);

    _quicklistP m_ptr {nullptr};
    InitParams m_params;
};

}
