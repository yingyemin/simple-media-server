/*
 * Copyright (c) 2016 The ZLToolKit project authors. All Rights Reserved.
 *
 * This file is part of ZLToolKit(https://github.com/ZLMediaKit/ZLToolKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "SrtEventLoop.h"
#include "Log/Logger.h"
#include "Util/TimeClock.h"
#include "Util/Thread.h"

#ifdef ENABLE_SRT

#ifndef _WIN32
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#endif

#include "srt/srt.h"


using namespace std;


#define EPOLL_SIZE 1024

static thread_local std::weak_ptr<SrtEventLoop> gCurrentLoop;

#ifndef _WIN32
static int createEventfd(){
    int evtfd = eventfd(0, EFD_NONBLOCK);
    if(evtfd < 0){
        logError << "createEventfd failed" << errno;
    }
    return evtfd;
}
#endif

SrtEventLoop::SrtEventLoop()
{
    _epollFd = srt_epoll_create();
    srt_epoll_set(_epollFd, SRT_EPOLL_ENABLE_EMPTY);
    _wakeupFd = srt_create_socket();
    logInfo << _wakeupFd;
}

SrtEventLoop::~SrtEventLoop()
{
    if (_epollFd != -1) {
        srt_epoll_release(_epollFd);
        _epollFd = -1;
    }
}

SrtEventLoop::Ptr SrtEventLoop::getCurrentLoop()
{
    logInfo << "EventLoop::getCurrentLoop(): " << gCurrentLoop.lock();
    return gCurrentLoop.lock();
}

void SrtEventLoop::start()
{
    Thread::setThreadName("srt-looper-" + to_string(_epollFd));
    int loopStrat = 0;

    gCurrentLoop = dynamic_pointer_cast<SrtEventLoop>(shared_from_this());
    logInfo << "EventLoop::start(): " << gCurrentLoop.lock();

    // addEvent(_wakeupFd, 0, [this](int event, void* args){ onAsyncEvent(); }, nullptr);
    _timer = make_shared<Timer>();

    _runTime = TimeClock::now();
    struct epoll_event events[EPOLL_SIZE];
    while (!_quit) {
        // if (_mapHander.size() == 0) {
        //     sleep(1);
        //     logInfo << "no fd add, continue";
        //     continue;
        // }
        loopStrat = TimeClock::now();

        uint64_t minDelay = _timer->flushTimerTask();
        _delayTaskDuration = TimeClock::now() - loopStrat;

        computeLoad();

        _waitTime = TimeClock::now();
        _lastRunDuration = _waitTime - _runTime;

        int read_len  = 1024;
        int write_len = 1024;
        SRTSOCKET  m_read_socks[1024];
        SRTSOCKET  m_write_socks[1024];
        // 此处没有好的办法对epoll进行唤醒，暂时没10ms唤醒一次，检查是否有异步任务执行
        int ret = srt_epoll_wait(_epollFd, m_read_socks, &read_len, m_write_socks, &write_len, 10, 0, 0, 0, 0);
        _runTime = TimeClock::now();

        // logInfo << "start async event";
        onAsyncEvent();

        if (ret < 0) {
            //超时或被打断
            continue;
        }
        _lastWaitDuration = _runTime - _waitTime;
        _fdCount = _mapHander.size();
        _timerTaskCount = _timer->getTaskSize();

        logInfo << "ret: " << ret << ", _mapHander.size(): " << _mapHander.size();
        for (int i = 0; i < read_len; ++i) {
            // struct epoll_event &ev = events[i];
            int fd = m_read_socks[i];
            auto it = _mapHander.find(fd);
            if (it == _mapHander.end()) {
                srt_epoll_remove_usock(_epollFd, fd);
                continue;
            }
            auto func = it->second.callback;
            try {
                func(EPOLLIN, it->second.args);
            } catch (std::exception &ex) {
                logWarn << "Exception occurred when do event task: " << ex.what();
            }
        }
        for (int i = 0; i < write_len; ++i) {
            // struct epoll_event &ev = events[i];
            int fd = m_write_socks[i];
            auto it = _mapHander.find(fd);
            if (it == _mapHander.end()) {
                srt_epoll_remove_usock(_epollFd, fd);
                continue;
            }
            auto func = it->second.callback;
            try {
                func(EPOLLIN, it->second.args);
            } catch (std::exception &ex) {
                logWarn << "Exception occurred when do event task: " << ex.what();
            }
        }
        _eventDuration = TimeClock::now() - _runTime;
    }
}

void SrtEventLoop::computeLoad()
{
    if (_waitTime == 0) {
        return ;
    }

    auto now = TimeClock::now();
    if (_waitTime > _runTime) {
        _curWaitDuration = now - _waitTime;
        _curRunDuration = 0;
    } else {
        _curWaitDuration = 0;
        _curRunDuration = now - _runTime;
        if (_curRunDuration > 1000) {
            // 大于1秒
            logWarn << "current loop(" << _epollFd << ") take time : " << _curRunDuration;
        }
    }

    float load = (float)_lastRunDuration / (_lastWaitDuration + _lastRunDuration);
}

void SrtEventLoop::getLoad(int& lastWaitDuration, int& lastRunDuration, int& curWaitDuration, int& curRunDuration)
{
    lastWaitDuration = _lastWaitDuration;
    lastRunDuration = _lastRunDuration;
    curWaitDuration = _curWaitDuration;
    curRunDuration = _curRunDuration;
}

void SrtEventLoop::setThread(thread* thd)
{
    _loopThread = thd;
}

bool SrtEventLoop::isCurrent()
{
    // return true;
    return !_loopThread || _loopThread->get_id() == this_thread::get_id();
}

void SrtEventLoop::addTimerTask(uint64_t ms, const TimerTask::timerHander &handler, TaskCompleteCB cb)
{
    if (!handler) {
        return ;
    }
    if (isCurrent()) {
        logInfo << "add timer";
        auto task = _timer->addTimer(ms, handler);
        if (cb) {
            cb(true, task);
        }
        return ;
    }
    
    async([this, ms, handler, cb](){
        addTimerTask(ms, handler, cb);
    }, true, false);
}

void SrtEventLoop::async(asyncEventFunc func, bool sync, bool front)
{
    if (sync && isCurrent()) {
        func();
        return ;
    }

    {
        lock_guard<mutex> lck(_mtxEvents);
        if (front) {
            _asyncEvents.emplace_front(func);
        } else {
            _asyncEvents.emplace_back(func);
        }
    }
    //写数据到管道,唤醒主线程
    // uint64_t  one = 1111;
    // ssize_t n = write(_wakeupFd, &one, sizeof(one));
    // if(n != sizeof(one)) {
    //     logWarn << "write wakeup Fd failed, n: " << n << ", _wakeupFd: " << _wakeupFd;
    // }

}

inline void SrtEventLoop::onAsyncEvent()
{
    if (_asyncEvents.size() == 0) {
        return ;
    }

    uint64_t  startTime = TimeClock::now();
    // uint64_t  one;
    // ssize_t size = 0;
    // while (true) {
    //     if ((size = read(_wakeupFd, &one, sizeof(one))) == sizeof(one)) {
    //         // 读到管道数据,继续读,直到读空为止
    //         logInfo << "read epoll fd";
    //         continue;
    //     }
    // }

    // read(_wakeupFd, &one, sizeof(one));

    decltype(_asyncEvents) _enventSwap;
    {
        lock_guard<mutex> lck(_mtxEvents);
        _enventSwap.swap(_asyncEvents);
    }

    for (auto& func : _enventSwap) {
        try {
            func();
            logInfo << "do a async event";
        } catch (std::exception &ex) {
            logWarn << "do async event failed: " << ex.what();
        }
    }
    _asyncEventDuration = TimeClock::now() - startTime;
}

int SrtEventLoop::addEvent(int fd, int event, SrtEventHander::eventCallback cb, void* args)
{
    if (!cb) {
        logInfo << "cb is empty" << endl;
        return -1;
    }
    // if (isCurrent()) {
        int ret = 0;
        int modes = event ? SRT_EPOLL_OUT : SRT_EPOLL_IN;

        modes |= SRT_EPOLL_ERR;
        ret = srt_epoll_add_usock(_epollFd, fd, &modes);
        if (ret < 0) {
            logError << "add to epoll failed, fd: " << fd;
            return srt_getlasterror(NULL);
        }
        logInfo << "add srt fd: " << fd;
        _mapHander[fd].callback = cb;
        _mapHander[fd].args = args;
        return ret;
    // }
    
    async([this, fd, event, cb, args](){
        addEvent(fd, event, cb, args);
    }, true, false);

    return 0;
}

void SrtEventLoop::delEvent(int fd, PollCompleteCB cb) {
    if (!cb) {
        cb = [](bool success) {};
    }

    if (isCurrent()) {
        int ret = 0;
        ret = srt_epoll_remove_usock(_epollFd, fd);
        if (ret < 0) {
            logError << "del from epoll failed, fd: " << fd;
        }

        _mapHander.erase(fd);
        cb(true);

        return ;
    }

    //跨线程操作
    async([this, fd, cb]() {
        delEvent(fd, std::move(const_cast<PollCompleteCB &>(cb)));
    }, true, false);
}

void SrtEventLoop::modifyEvent(int fd, int event, PollCompleteCB cb) {
    if (!cb) {
        cb = [](bool success) {};
    }
    if (isCurrent()) {
        int ret = 0;
        int modes = event ? SRT_EPOLL_OUT : SRT_EPOLL_IN;

        modes |= SRT_EPOLL_ERR;
        ret = srt_epoll_update_usock(_epollFd, fd, &modes);
        if (ret < 0) {
            logError << "del from epoll failed, fd: " << fd;
        }

        _mapHander.erase(fd);
        cb(true);

        return ;
    }
    async([this, fd, event, cb]() {
        modifyEvent(fd, event, std::move(const_cast<PollCompleteCB &>(cb)));
    }, true, false);
}

#endif