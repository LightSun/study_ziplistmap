#include <string>
#include "h7_redis.hpp"
#include "ZipList.h"
#include "QuickList.h"
#include "redisassert.h"

using namespace h7;
using String = std::string;

static void test_Ziplist();
static void test_QuickList();

extern void test_dna_compress();
extern void test_JudyList();
extern void test_compressor();
extern void test_buffer_io();

int main(int argc, char* argv[]){
    setbuf(stdout, NULL);
    //test string if copy.
//    const char* s = "abc";
//    std::string str(s);
//    printf("p1 = %p, p2 = %p\n", s, str.data());

    //int ret = ziplistTest(argc, argv);
    //int ret = zipmapTest(argc, argv);

    test_Ziplist();
    test_QuickList();
    //judy array on windows have bug.
    //test_JudyList();

    test_buffer_io();
    //test_dna_compress();
    //test_compressor();
    return 0;
}

void test_Ziplist(){
    ZipList list;
    list.add("abc");
    list.add("def");
    list.add("gh");
    assert(list.size() == 3);
    list.removeAt(1);
    assert(list.size() == 2);
    assert(list.get(0) == "abc");
    assert(list.get(1) == "gh");
    assert(list.get(10) == "");

    assert(list.indexOf("abc") == 0);
    assert(list.indexOf("gh") == 1);
    list.set(1, "12345");

    auto s1 = list.get(0);
    auto s2 = list.get(1);
    //printf("test_Ziplist >> s1 = %s\n", s1.data());
    //printf("test_Ziplist >> s2 = %s\n", s2.data());
    assert(list.get(0) == "abc");
    assert(list.get(1) == "12345");
    //
}

void test_QuickList(){
    QuickList list;
    list.add("abc");
    list.add("def");
    list.add("gh");
    assert(list.size() == 3);
    list.removeAt(1);
    assert(list.size() == 2);
    assert(list.get(0) == "abc");
    assert(list.get(1) == "gh");

    assert(list.indexOf("abc") == 0);
    assert(list.indexOf("gh") == 1);
    list.set(1, "12345");

    auto s1 = list.get(0);
    auto s2 = list.get(1);
    assert(list.get(0) == "abc");
    assert(list.get(1) == "12345");
}
