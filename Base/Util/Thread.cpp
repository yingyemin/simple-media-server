#include "Thread.h"


#include <cstring>
#include <assert.h>
#include <thread>

#ifndef _WIN32
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>

thread_local std::string g_threadName;
thread_local int g_tid = -1;
#else
#include <Windows.h>
#include <thread>
#include <unordered_map>
#include <mutex>
std::unordered_map<DWORD, std::string> threadNames;
std::mutex nameMutex;
#endif

using namespace std;



int Thread::getThreadId()
{
    
#ifdef _WIN32
    return GetCurrentThreadId();
#else
    if (g_tid != -1) {
        return g_tid;
    }
    g_tid = syscall(SYS_gettid);
    return g_tid;
#endif
}

string Thread::getThreadName()
{
#ifndef _WIN32
    if (!g_threadName.empty()) {
        return g_threadName;
    }
    
    string name;
    name.resize(32);
    auto tid = pthread_self();
    pthread_getname_np(tid, (char *) name.data(), name.size());
    if (name[0]) {
        name.resize(strlen(name.data()));
        return name;
    }
    return to_string((uint64_t) tid);
#else
    std::lock_guard<std::mutex> lock(nameMutex);
    DWORD threadId = GetCurrentThreadId();
    auto it = threadNames.find(threadId);
    if (it != threadNames.end()) {
        return it->second;
    }
    return std::to_string(threadId);
#endif
}

void Thread::setThreadName(const string& name)
{
    assert(name.size() < 16); // linux平台下线程名字长度需小于16
#ifndef _WIN32
    pthread_setname_np(pthread_self(), name.data());
#else
    std::lock_guard<std::mutex> lock(nameMutex);
    DWORD threadId = GetCurrentThreadId();
    threadNames[threadId] = name;
#endif
}