
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
include_directories(./)

file(GLOB_RECURSE SRC_QUIP_ALL  src/*.cpp src/*.h src/*.c src/*.cpp)
file(GLOB_RECURSE SRC_EXCLUDE  src/dist_impl.c
    src/quip.c
    src/test_quip.cpp
    )

list(REMOVE_ITEM SRC_QUIP_ALL ${SRC_EXCLUDE})

## lib
add_library(quip
    ${SRC_QUIP_ALL}
    )

target_link_libraries(quip PUBLIC z bz2 pthread)

## fqc
file(GLOB_RECURSE SRC_MED_COMPRESS_ALL  fqc/*.cpp fqc/*.h
    fqc/*.c fqc/*.hpp)
add_library(fqcompress
    ${SRC_MED_COMPRESS_ALL}
    )
target_compile_definitions(fqcompress PUBLIC USE_C11_ATOMICS)
target_link_libraries(fqcompress PUBLIC ${DEP_LIBS} quip)

## demo
add_executable(test_quip
    src/quip.c
    src/test_quip.cpp
    )
target_link_libraries(test_quip PUBLIC quip)
