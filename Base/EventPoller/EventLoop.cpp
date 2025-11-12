/*
 * Copyright (c) 2016 The ZLToolKit project authors. All Rights Reserved.
 *
 * This file is part of ZLToolKit(https://github.com/ZLMediaKit/ZLToolKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */
#include "EventLoop.h"
#include "Log/Logger.h"
#include "Util/TimeClock.h"
#include "Util/Thread.h"

#ifndef _WIN32
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#endif

using namespace std;

#define EPOLL_SIZE 1024

static thread_local std::weak_ptr<EventLoop> gCurrentLoop;

#ifndef _WIN32
static int createEventfd(){
    int evtfd = eventfd(0, EFD_NONBLOCK);
    if(evtfd < 0){
        logError << "createEventfd failed" << errno;
    }
    return evtfd;
}
#endif

#if defined(_WIN32)
int setNoBlocked(int fd, bool noblock) {
#if defined(_WIN32)
    unsigned long ul = noblock;
#else
    int ul = noblock;
#endif //defined(_WIN32)
    int ret = ioctlsocket(fd, FIONBIO, &ul); //设置为非阻塞模式
    if (ret == -1) {
        logTrace << "ioctl FIONBIO failed";
    }

    return ret;
}
#endif

EventLoop::EventLoop()
{
    _epollFd = epoll_create(EPOLL_SIZE);

#if USE_EPOLL_WAKEUP
#ifdef _WIN32
    setNoBlocked(_wakeupFd.readFD());
    setNoBlocked(_wakeupFd.writeFD());
#else
    _wakeupFd = createEventfd();
    logInfo << "_wakeupFd: " << _wakeupFd;
#endif
#endif // USE_EPOLL_WAKEUP
    /*for (int i = 0; i < 10; i++) {
        shared_ptr<ReaderWriterQueue<asyncEventFunc>> que = make_shared<ReaderWriterQueue<asyncEventFunc>>(10);
        _asyncQueues.push_back(que);
    }*/
}

EventLoop::~EventLoop()
{
#ifdef _WIN32
    if (_epollFd)
        epoll_close(_epollFd);
#else
#if USE_EPOLL_WAKEUP
    if (_wakeupFd != -1) {
        close(_wakeupFd);
        _wakeupFd = -1;
    }
#endif // USE_EPOLL_WAKEUP
    
    if (_epollFd != -1) {
        close(_epollFd);
        _epollFd = -1;
    }
#endif
    if (_loopThread) {
        delete _loopThread;
    }
}

// 获取后需要判空
EventLoop::Ptr EventLoop::getCurrentLoop()
{
    // logInfo << "EventLoop::getCurrentLoop(): " << gCurrentLoop.lock();
    return gCurrentLoop.lock();
}

void EventLoop::start()
{
    Thread::setThreadName("looper-" + std::to_string(_epollFd));
    _threadId = Thread::getThreadId();
    int loopStrat = 0;

    gCurrentLoop = shared_from_this();
    logInfo << "EventLoop::start(): " << gCurrentLoop.lock();

#if USE_EPOLL_WAKEUP
#ifndef _WIN32
    addEvent(_wakeupFd, EPOLLIN, [this](int event, void* args){ onAsyncEvent(); }, nullptr);
#else
    addEvent(_wakeupFd.readFD(), EPOLLIN, [this](int event, void* args) { onAsyncEvent(); }, nullptr);
#endif
#endif // USE_EPOLL_WAKEUP
    _timer = make_shared<Timer>();

    _runTime = TimeClock::now();
    struct epoll_event events[EPOLL_SIZE];
    while (!_quit) {
        loopStrat = TimeClock::now();

        uint64_t minDelay = _timer->flushTimerTask();
#if !USE_EPOLL_WAKEUP
        onAsyncEvent();
#endif
        // logTrace << "next epoll run time: " << minDelay;
        _delayTaskDuration = TimeClock::now() - loopStrat;

        computeLoad();

        _eventRun = false;
        _waitTime = TimeClock::now();
        // logTrace << "_waitTime: " << _waitTime;
        _lastRunDuration = _waitTime - _runTime;

        // int ret = epoll_wait(_epollFd, events, EPOLL_SIZE, minDelay ? minDelay : -1);
        int ret = epoll_wait(_epollFd, events, EPOLL_SIZE, minDelay < 40 ? minDelay : 40);

        _eventRun = true;
        _runTime = TimeClock::now();
        // logTrace << "_runTime: " << _runTime;
        _lastWaitDuration = _runTime - _waitTime;

        _fdCount = _mapHander.size();
        _timerTaskCount = _timer->getTaskSize();

        if (ret <= 0) {
            //超时或被打断
            continue;
        }
        
        // logInfo << "ret: " << ret << ", _mapHander.size(): " << _mapHander.size();
        for (int i = 0; i < ret; ++i) {
            struct epoll_event &ev = events[i];
            int fd = ev.data.fd;
            auto it = _mapHander.find(fd);
            if (it == _mapHander.end()) {
                epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, nullptr);
                continue;
            }
            auto func = it->second.callback;
            try {
                func(ev.events, it->second.args);
            } catch (std::exception &ex) {
                logWarn << "Exception occurred when do event task: " << ex.what();
            }
        }
        _eventDuration = TimeClock::now() - _runTime;
    }
}

void EventLoop::computeLoad()
{
    if (_waitTime == 0) {
        return ;
    }

    auto now = TimeClock::now();
    if (!_eventRun) {
        _curWaitDuration = now - _waitTime;
        _curRunDuration = 0;
    } else {
        _curWaitDuration = 0;
        _curRunDuration = now - _runTime;
        if (_curRunDuration > 1000) {
            // 大于1秒
            logWarn << "current loop(" << (int)_epollFd << ") take time : " << _curRunDuration;
        }
    }

    float load = (float)_lastRunDuration / (_lastWaitDuration + _lastRunDuration);
}

void EventLoop::getLoad(int& lastWaitDuration, int& lastRunDuration, int& curWaitDuration, int& curRunDuration)
{
    auto now = TimeClock::now();
    if (!_eventRun) {
        _curWaitDuration = now - _waitTime;
        _curRunDuration = 0;
    } else {
        _curWaitDuration = 0;
        _curRunDuration = now - _runTime;
    }

    lastWaitDuration = _lastWaitDuration;
    lastRunDuration = _lastRunDuration;
    curWaitDuration = _curWaitDuration;
    curRunDuration = _curRunDuration;
}

void EventLoop::setThread(thread* thd)
{
    _loopThread = thd;
}

bool EventLoop::isCurrent()
{
    return !_loopThread || _loopThread->get_id() == std::this_thread::get_id();
}

void EventLoop::addTimerTask(uint64_t ms, const TimerTask::timerHander &handler, TaskCompleteCB cb)
{
    if (!handler) {
        return ;
    }
    if (isCurrent()) {
        logTrace << "add timer";
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

void EventLoop::async(asyncEventFunc func, bool sync, bool front)
{
    // logInfo << "EventLoop::async";
    if (sync && isCurrent()) {
        func();
        return ;
    }
#if 1
    {
        lock_guard<mutex> lck(_mtxEvents);
        if (front) {
            _asyncEvents.emplace_front(func);
        } else {
            _asyncEvents.emplace_back(func);
        }
    }
#else
    auto loop = EventLoop::getCurrentLoop();
    int id = 9;
    if (loop) {
        id = loop->getEpollID();
    }
    _asyncQueues[id]->enqueue(func);
#endif

#if USE_EPOLL_WAKEUP
    uint64_t  one = 1111;
#ifndef _WIN32
    // 写数据到管道,唤醒主线程
    ssize_t n = write(_wakeupFd, &one, sizeof(one));
    if (n != sizeof(one)) {
        logWarn << "write wakeup Fd failed, n: " << n << ", _wakeupFd: " << _wakeupFd;
    }
#else
    ssize_t n = _wakeupFd.write(&one, sizeof(one));
    if (n != sizeof(one)) {
        logWarn << "write wakeup Fd failed, n: " << n << ", _wakeupFd: " << _wakeupFd.writeFD();
    }
#endif
#endif // USE_EPOLL_WAKEUP
}

inline void EventLoop::onAsyncEvent()
{
    uint64_t  startTime = TimeClock::now();
    uint64_t  one;

#if USE_EPOLL_WAKEUP
#ifndef _WIN32
    read(_wakeupFd, &one, sizeof(one));
#else
    _wakeupFd.read(&one, sizeof(one));
#endif
#endif // USE_EPOLL_WAKEUP

#if 1
    decltype(_asyncEvents) _enventSwap;
    {
        lock_guard<mutex> lck(_mtxEvents);
        _enventSwap.swap(_asyncEvents);
    }

    for (auto& func : _enventSwap) {
        try {
            func();
        } catch (std::exception &ex) {
            logWarn << "do async event failed: " << ex.what();
        }
    }
#else
    bool success;
    asyncEventFunc func;
    for (auto& queue: _asyncQueues) {
        while(queue->size_approx()) {
            success = queue->try_dequeue(func);
            if (success) {
                try {
                    func();
                } catch (std::exception &ex) {
                    logWarn << "do async event failed: " << ex.what();
                }
            } else {
                break;
            }
        }
    }
#endif
    _asyncEventDuration = TimeClock::now() - startTime;
}

int EventLoop::addEvent(int fd, int event, EventHander::eventCallback cb, void* args)
{
    if (!cb) {
        logInfo << "cb is empty" << endl;
        return -1;
    }
    if (isCurrent()) {
        struct epoll_event ev = {0};
        ev.events = event;
        ev.data.fd = fd;
        int ret = epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &ev);
        if (ret == 0) {
            _mapHander[fd].callback = cb;
            _mapHander[fd].args = args;
        }
        logDebug << "ret=" << ret << ", fd=" << fd;
       // printf("ret=%d",ret);
        return ret;
    }
    
    async([this, fd, event, cb, args](){
        addEvent(fd, event, cb, args);
    }, true, false);

    return 0;
}

void EventLoop::delEvent(int fd, PollCompleteCB cb) {
    if (!cb) {
        cb = [](bool success) {};
    }

    if (isCurrent()) {
        if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, nullptr) != 0) {
            cb(false);
            return ;
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

void EventLoop::modifyEvent(int fd, int event, PollCompleteCB cb) {
    if (!cb) {
        cb = [](bool success) {};
    }
    if (isCurrent()) {
        struct epoll_event ev = { 0 };
        ev.events = event;
        ev.data.fd = fd;
        if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) != 0) {
            cb(false);
            return ;
        }
        cb(true);
        return ;
    }
    async([this, fd, event, cb]() {
        modifyEvent(fd, event, std::move(const_cast<PollCompleteCB &>(cb)));
    }, true, false);
}
