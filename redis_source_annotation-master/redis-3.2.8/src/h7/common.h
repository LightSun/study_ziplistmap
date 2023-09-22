#pragma once

#include <string>
#include <iostream>

#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)
#endif

#define ASSERT(x, fmt, ...) \
do{\
    if(!(x)){\
        /*fprintf(stderr, fmt, ##__VA_ARGS__);*/\
        char buf[1024];\
        snprintf(buf, 1024, fmt, ##__VA_ARGS__);\
        std::cout << buf << std::endl;\
        abort();\
    }\
}while(0);

#ifdef _WIN32
#define CMD_LINE "\r\n"
#else
#define CMD_LINE "\n"
#endif

#ifndef String
using String = std::string;
#endif

#ifndef CString
using CString = const std::string&;
#endif

#define LOGE(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
