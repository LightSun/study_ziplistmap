cmake_minimum_required(VERSION 3.5)

## first install pcre2
##

project(stu_ziplist LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

##
add_compile_definitions(REDIS_TEST)


include_directories(./)
add_library(redis
    zmalloc.h
    zmalloc.c
    ziplist.h
    ziplist.c
    quicklist.h
    quicklist.c
    util.h
    util.c
   # sha1.h
    sha1.c
    sds.h
    sds.c
    sdsalloc.h
    endianconv.h
    endianconv.c
    redisassert.h
    lzf.h
    lzf_c.c
    lzf_d.c
    adlist.c
    zipmap.h
    zipmap.c
    h7_redis.hpp
    )

file(GLOB_RECURSE SRC_H7_TEST  h7/*.cpp h7/*.h h7/*.hpp)

add_executable(test_redis
    ${SRC_H7_TEST}
    )
target_link_libraries(test_redis PUBLIC redis)
