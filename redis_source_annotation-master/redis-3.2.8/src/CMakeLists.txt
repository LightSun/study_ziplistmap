cmake_minimum_required(VERSION 3.5)

## first install pcre2
##

project(stu_ziplist LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

## judy

set(JUDY_DIR $ENV{JUDY_DIR})
if(NOT DEFINED ENV{JUDY_DIR})
    if(CMAKE_HOST_WIN32)
        set(JUDY_DIR D:/study/cpp/ENV/judy)
    else()
        set(JUDY_DIR /home/heaven7/heaven7/env/linux/judy)
    endif()
endif()
include_directories(${JUDY_DIR}/include)
SET(JUDY_LIBS
    ${JUDY_DIR}/libs/libJudy.a
    )

## zstd
SET(ZSTD_LIBS)
if(CMAKE_HOST_WIN32)
    # mingw
    SET(ZSTD_LIBS zstd)
else(CMAKE_HOST_WIN32)
    set(ZSTD_DIR $ENV{ZSTD_DIR})
    if(NOT DEFINED ENV{ZSTD_DIR})
        set(ZSTD_DIR /home/heaven7/heaven7/env/linux/zstd)
    endif()
    include_directories(${ZSTD_DIR}/include)
    SET(ZSTD_LIBS
        ${ZSTD_DIR}/libs/libzstd.a  # static for single thread. so for multi
        )
endif()

##
set(DEP_LIBS
    ${JUDY_LIBS}
    ${ZSTD_LIBS}
    z
    pthread
    )

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

#

file(GLOB_RECURSE SRC_H7_TEST  h7/*.cpp h7/*.h h7/*.c h7/*.hpp)

add_executable(test_redis
    ${SRC_H7_TEST}
    )
target_link_libraries(test_redis PUBLIC redis ${DEP_LIBS})
