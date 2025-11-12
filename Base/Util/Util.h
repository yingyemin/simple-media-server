/*
 * Copyright (c) 2016 The ZLToolKit project authors. All Rights Reserved.
 *
 * This file is part of ZLToolKit(https://github.com/ZLMediaKit/ZLToolKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef UTIL_UTIL_H_
#define UTIL_UTIL_H_
#if defined(_WIN32)

#include <ctime>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <atomic>
#include <unordered_map>

#if defined(_WIN32)

#define UTIL_PATH_MAX 1024
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment (lib,"WS2_32")
#include <stdint.h>

enum EPOLL_EVENTS {
    EPOLLIN = (int)(1U << 0),
    EPOLLPRI = (int)(1U << 1),
    EPOLLOUT = (int)(1U << 2),
    EPOLLERR = (int)(1U << 3),
    EPOLLHUP = (int)(1U << 4),
    EPOLLRDNORM = (int)(1U << 6),
    EPOLLRDBAND = (int)(1U << 7),
    EPOLLWRNORM = (int)(1U << 8),
    EPOLLWRBAND = (int)(1U << 9),
    EPOLLMSG = (int)(1U << 10), /* Never reported. */
    EPOLLRDHUP = (int)(1U << 13),
    EPOLLONESHOT = (int)(1U << 31)
};

#define EPOLLIN      (1U <<  0)
#define EPOLLPRI     (1U <<  1)
#define EPOLLOUT     (1U <<  2)
#define EPOLLERR     (1U <<  3)
#define EPOLLHUP     (1U <<  4)
#define EPOLLRDNORM  (1U <<  6)
#define EPOLLRDBAND  (1U <<  7)
#define EPOLLWRNORM  (1U <<  8)
#define EPOLLWRBAND  (1U <<  9)
#define EPOLLMSG     (1U << 10)
#define EPOLLRDHUP   (1U << 13)
#define EPOLLONESHOT (1U << 31)

#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_MOD 2
#define EPOLL_CTL_DEL 3

typedef void* HANDLE;
typedef uintptr_t SOCKET;

typedef union epoll_data {
    void* ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
    SOCKET sock; /* Windows specific */
    HANDLE hnd;  /* Windows specific */
} epoll_data_t;

struct epoll_event {
    uint32_t events;   /* Epoll events and flags */
    epoll_data_t data; /* User data variable */
};

#include "Wepoll.h"
#else
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <cstddef>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#endif // defined(_WIN32)

#define INSTANCE_IMP(class_name, ...) \
class_name &class_name::Instance() { \
    static std::shared_ptr<class_name> s_instance(new class_name(__VA_ARGS__)); \
    static class_name &s_instance_ref = *s_instance; \
    return s_instance_ref; \
}


#if defined(_WIN32)
#if !defined(strcasecmp)
    #define strcasecmp _stricmp
#endif

#if !defined(strncasecmp)
#define strncasecmp _strnicmp
#endif

#ifndef ssize_t
    #ifdef _WIN64
        #define ssize_t int64_t
    #else
        #define ssize_t int32_t
    #endif
#endif
#endif

#ifndef bzero
#define bzero(ptr,size)  memset((ptr),0,(size));
#endif //bzero

#ifdef _WIN32
int gettimeofday(struct timeval* tp, void* tzp);
#endif // _WIN32
/**
 * 获取时间差, 返回值单位为秒
 * Get time difference, return value in seconds

 * [AUTO-TRANSLATED:43d2403a]
 */
long getGMTOff();
std::string exePath(bool isExe = true);
std::string exeDir(bool isExe = true);
std::string exeName(bool isExe = true);
//字符串是否以xx开头  [AUTO-TRANSLATED:585cf826]
//Check if a string starts with xx
bool start_with(const std::string& str, const std::string& substr);
//字符串是否以xx结尾  [AUTO-TRANSLATED:50cc80d7]
//Check if a string ends with xx
bool end_with(const std::string& str, const std::string& substr);
std::vector<std::string> split(const std::string& s, const char* delim);

#if defined(__MACH__)
#elif defined(__linux__)
#include <arpa/inet.h>
#include <endian.h>
#elif defined(_WIN32)
#define BIG_ENDIAN 1
#define LITTLE_ENDIAN 0
#define BYTE_ORDER LITTLE_ENDIAN
#define __BYTE_ORDER BYTE_ORDER
#define __BIG_ENDIAN BIG_ENDIAN
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#endif
#endif // define Win

#endif /* UTIL_UTIL_H_ */
