/*
 * wepoll - epoll for Windows
 * https://github.com/piscisaureus/wepoll
 *
 * Copyright 2012-2020, Bert Belder <bertbelder@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WEPOLL_H_
#define WEPOLL_H_

#if defined(_WIN32)

#ifndef WEPOLL_EXPORT
#define WEPOLL_EXPORT
#endif
#include <map>
#include <mutex>
#include "Util/Util.h"

#ifdef __cplusplus
extern "C" {
#endif

WEPOLL_EXPORT HANDLE epoll_create_my(int size);
WEPOLL_EXPORT HANDLE epoll_create1_my(int flags);

WEPOLL_EXPORT int epoll_close_my(HANDLE ephnd);

WEPOLL_EXPORT int epoll_ctl_my(HANDLE ephnd,
                            int op,
                            SOCKET sock,
                            struct epoll_event* event);

WEPOLL_EXPORT int epoll_wait_my(HANDLE ephnd,
                             struct epoll_event* events,
                             int maxevents,
                             int timeout);

#ifdef __cplusplus
} /* extern "C" */
#endif


// 索引handle
extern std::map<int, HANDLE> s_wepollHandleMap;
extern int s_handleIndex;
extern std::mutex s_handleMtx;
// 屏蔽epoll_create epoll_ctl epoll_wait参数差异
inline int epoll_create(int size) {
    HANDLE handle = ::epoll_create_my(size);
    if (!handle) {
        return -1;
    }
    {
        std::lock_guard<std::mutex> lck(s_handleMtx);
        int idx = ++s_handleIndex;
        s_wepollHandleMap[idx] = handle;
        return idx;
    }
}

inline int epoll_ctl(int ephnd, int op, SOCKET sock, struct epoll_event* ev) {
    HANDLE handle;
    {
        std::lock_guard<std::mutex> lck(s_handleMtx);
        handle = s_wepollHandleMap[ephnd];
    }
    return ::epoll_ctl_my(handle, op, sock, ev);
}

inline int epoll_wait(int ephnd, struct epoll_event* events, int maxevents, int timeout) {
    HANDLE handle;
    {
        std::lock_guard<std::mutex> lck(s_handleMtx);
        handle = s_wepollHandleMap[ephnd];
    }
    return ::epoll_wait_my(handle, events, maxevents, timeout);
}

inline int epoll_close(int ephnd) {
    HANDLE handle = nullptr;
    {
        std::lock_guard<std::mutex> lck(s_handleMtx);
        auto it = s_wepollHandleMap.find(ephnd);
        if (it != s_wepollHandleMap.end())
        {
            handle = it->second;
            s_wepollHandleMap.erase(it);
        }
    }
    return ::epoll_close_my(handle);
}


#endif
#endif /* WEPOLL_H_ */
