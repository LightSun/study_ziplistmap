
#include "JudyList.h"
#include "redisassert.h"
//
using namespace h7;

void test_JudyList(){
    JudyList list;
    list.add("abcdef2342434");
    list.add("abc2");
    list.add("abc3");
    list.add("abc4");
    list.add(0, "abc");

    int idx = list.indexOf("abcdef2342434");
    //printf("idx = %d \n", idx);
    assert(idx == 1);

    idx = list.indexOf("gfhfghfh");
    assert(idx == -1);

    list.print();
//    for(int i = 0 ; i < list.size() ; ++i){
//        auto str = list.get(i);
//        printf("id = %d, data =%s\n", i, str.data());
//    }

    list.removeAt(2);
    printf("------ after removeAt(2) -----\n");
//    for(int i = 0 ; i < list.size() ; ++i){
//        auto str = list.get(i);
//        printf("id = %d, data =%s\n", i, str.data());
//    }
    list.print();
}
