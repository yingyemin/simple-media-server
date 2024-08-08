#include "Thread.h"

#include <pthread.h>
#include <cstring>
#include <assert.h>
#include <thread>
#include <unistd.h>
#include <sys/syscall.h>

using namespace std;

thread_local string g_threadName;
thread_local int g_tid = -1;

int Thread::getThreadId()
{
    if (g_tid != -1) {
        return g_tid;
    }

    g_tid = syscall(SYS_gettid);
    return g_tid;
}

string Thread::getThreadName()
{
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
}

void Thread::setThreadName(const string& name)
{
    assert(name.size() < 16); // linux平台下线程名字长度需小于16
    pthread_setname_np(pthread_self(), name.data());
}