/*
 * Copyright (c) 2016 The ZLToolKit project authors. All Rights Reserved.
 *
 * This file is part of ZLToolKit(https://github.com/ZLMediaKit/ZLToolKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#if defined(ENABLE_URING)

#include "UringLoop.h"
#include "Log/Logger.h"
#include "Util/TimeClock.h"
#include "Util/Thread.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <poll.h>
#include <sys/epoll.h>
#include <cstring>

using namespace std;

#define IO_URING_ENTRIES 4096
#define BUFFER_SIZE 1024
#define BATCH_SIZE 32

enum { EVENT_ACCEPT, EVENT_READ, EVENT_WRITE };

struct conn_info {
    int fd;
    int type;
};

static thread_local std::weak_ptr<UringLoop> gCurrentLoop;

static int createEventfd(){
    int evtfd = eventfd(0, EFD_NONBLOCK);
    if(evtfd < 0){
        logError << "createEventfd failed" << errno;
    }
    return evtfd;
}

void set_accept_event(struct io_uring *ring, int fd) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_accept(sqe, fd, NULL, NULL, 0);
    struct conn_info ci = { .fd = fd, .type = EVENT_ACCEPT };
    memcpy(&sqe->user_data, &ci, sizeof(ci));
}

void set_read_event(struct io_uring *ring, int fd, char *buf) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_recv(sqe, fd, buf, BUFFER_SIZE, 0);
    struct conn_info ci = { .fd = fd, .type = EVENT_READ };
    memcpy(&sqe->user_data, &ci, sizeof(ci));
}

void set_write_event(struct io_uring *ring, int fd, char *buf, int len) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_send(sqe, fd, buf, len, 0);
    struct conn_info ci = { .fd = fd, .type = EVENT_WRITE };
    memcpy(&sqe->user_data, &ci, sizeof(ci));
}

UringLoop::UringLoop()
{
    // 初始化io_uring
    int ret = io_uring_queue_init(IO_URING_ENTRIES, &_ring, 0);
    if (ret < 0) {
        logError << "io_uring_queue_init failed: " << ret;
        // 可以在这里添加fallback到epoll的逻辑
    }
    
    // 创建唤醒fd
    _wakeupFd = eventfd(0, EFD_NONBLOCK);
    if(_wakeupFd < 0){
        logError << "create wakeup fd failed" << errno;
    }

    // 正确初始化_io_uring_fd
    _io_uring_fd = _ring.ring_fd;
    
    // 将唤醒fd添加到io_uring监控
    struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
    if (sqe) {
        io_uring_prep_poll_add(sqe, _wakeupFd, POLLIN);
        struct conn_info ci = { .fd = _wakeupFd, .type = EVENT_READ };
        memcpy(&sqe->user_data, &ci, sizeof(ci));
        io_uring_submit(&_ring);
    }
    
    logInfo << "_wakeupFd: " << _wakeupFd;
}

UringLoop::~UringLoop()
{
    if (_wakeupFd != -1) {
        close(_wakeupFd);
        _wakeupFd = -1;
    }

    if (_loopThread) {
        delete _loopThread;
    }
    
    // 清理io_uring
    io_uring_queue_exit(&_ring);
}

// 获取后需要判空
UringLoop::Ptr UringLoop::getCurrentLoop()
{
    return gCurrentLoop.lock();
}

void UringLoop::start()
{
    Thread::setThreadName("looper-" + to_string(_io_uring_fd));
    int loopStrat = 0;

    gCurrentLoop = dynamic_pointer_cast<UringLoop>(shared_from_this());
    logInfo << "UringLoop::start(): " << gCurrentLoop.lock();
    
    _timer = make_shared<Timer>();

    _runTime = TimeClock::now();
    
    while (!_quit) {
        loopStrat = TimeClock::now();

        uint64_t minDelay = _timer->flushTimerTask();
        
        // 处理异步事件
        onAsyncEvent();
        
        _delayTaskDuration = TimeClock::now() - loopStrat;

        computeLoad();

        _eventRun = false;
        _waitTime = TimeClock::now();
        _lastRunDuration = _waitTime - _runTime;

        // 设置io_uring_wait的超时时间
        struct __kernel_timespec ts = {0};
        bool use_timeout = false;
        if (minDelay > 0) {
            use_timeout = true;
            ts.tv_sec = minDelay / 1000;
            ts.tv_nsec = (minDelay % 1000) * 1000000;
        }

        // 等待io_uring事件
        struct io_uring_cqe *cqe;
        unsigned head;
        int ret = 0;
        
        // 使用带超时的peek_batch_cqe来处理多个事件
        if (use_timeout) {
            ret = io_uring_wait_cqe_timeout(&_ring, &cqe, &ts);
        } else {
            ret = io_uring_wait_cqe(&_ring, &cqe);
        }

        _eventRun = true;
        _runTime = TimeClock::now();
        _lastWaitDuration = _runTime - _waitTime;

        _fdCount = _mapHander.size();
        _timerTaskCount = _timer->getTaskSize();

        if (ret < 0) {
            // 错误或被中断
            if (ret != -ETIME && ret != -EINTR) {
                logError << "io_uring_wait_cqe failed: " << ret;
            }
            continue;
        }

        // 批量处理所有完成的事件
        int count = 0;
        io_uring_for_each_cqe(&_ring, head, cqe) {
            if (!cqe) continue;

            ++count;
            
            struct conn_info ci;
            memcpy(&ci, &cqe->user_data, sizeof(ci));
            int fd = ci.fd;
            int type = ci.type;
            uint32_t res_events = cqe->res;
            
            // 如果是唤醒fd，读取数据但不重新注册事件（会在async方法中处理）
            if (fd == _wakeupFd) {
                uint64_t one;
                read(_wakeupFd, &one, sizeof(one));
            } else {
                // 处理普通fd事件
                auto it = _mapHander.find(fd);
                if (it != _mapHander.end()) {
                    auto func = it->second.callback;
                    try {
                        func(res_events, it->second.args);
                    } catch (std::exception &ex) {
                        logWarn << "Exception occurred when do event task: " << ex.what();
                    }
                }
                
                // 检查fd是否仍然在监控中，如果是则重新注册事件
                if (_mapHander.find(fd) != _mapHander.end()) {
                    auto event_it = _fd_to_event.find(fd);
                    if (event_it != _fd_to_event.end()) {
                        struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
                        if (sqe) {
                            io_uring_prep_poll_add(sqe, fd, event_it->second);
                            struct conn_info new_ci = { .fd = fd, .type = type };
                            memcpy(&sqe->user_data, &new_ci, sizeof(new_ci));
                            io_uring_submit(&_ring);
                        }
                    }
                }
            }
        }
        
        // 标记所有CQE为已处理
        io_uring_cq_advance(&_ring, count);
        
        _eventDuration = TimeClock::now() - _runTime;
    }
}

void UringLoop::submit_io_uring_cqe()
{
    // 提交所有待处理的SQE
    int submitted = io_uring_submit(&_ring);
    if (submitted < 0) {
        logError << "io_uring_submit failed: " << submitted;
    }
}

void UringLoop::computeLoad()
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
            logWarn << "current loop(" << _io_uring_fd << ") take time : " << _curRunDuration;
        }
    }

    float load = (float)_lastRunDuration / (_lastWaitDuration + _lastRunDuration);
}

void UringLoop::getLoad(int& lastWaitDuration, int& lastRunDuration, int& curWaitDuration, int& curRunDuration)
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

void UringLoop::setThread(thread* thd)
{
    _loopThread = thd;
}

bool UringLoop::isCurrent()
{
    return !_loopThread || _loopThread->get_id() == this_thread::get_id();
}

void UringLoop::addTimerTask(uint64_t ms, const TimerTask::timerHander &handler, TaskCompleteCB cb)
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

void UringLoop::async(asyncEventFunc func, bool sync, bool front)
{
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
    auto loop = UringLoop::getCurrentLoop();
    int id = 9;
    if (loop) {
        id = loop->getEpollID();
    }
    _asyncQueues[id]->enqueue(func);
#endif

    // 写数据到唤醒fd，触发io_uring事件
    uint64_t one = 1111;
    ssize_t n = write(_wakeupFd, &one, sizeof(one));
    if(n != sizeof(one)) {
        logWarn << "write wakeup Fd failed, n: " << n << ", _wakeupFd: " << _wakeupFd;
    }
}

inline void UringLoop::onAsyncEvent()
{
    uint64_t startTime = TimeClock::now();
    
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

int UringLoop::addEvent(int fd, int event, EventHander::eventCallback cb, void* args)
{
    if (!cb) {
        logInfo << "cb is empty" << endl;
        return -1;
    }
    if (isCurrent()) {
        // 将epoll事件转换为io_uring poll事件
        uint32_t poll_events = 0;
        int type = 0;
        if (event & EPOLLIN) {
            poll_events |= POLLIN;
            type = EVENT_READ;
        }
        // if (event & EPOLLOUT) {
        //     poll_events |= POLLOUT;
        //     type = EVENT_WRITE;
        // }
        if (event & EPOLLPRI) poll_events |= POLLPRI;
        if (event & EPOLLERR) poll_events |= POLLERR;
        if (event & EPOLLHUP) poll_events |= POLLHUP;
        
        // 添加到io_uring
        struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
        if (!sqe) {
            logError << "io_uring_get_sqe failed";
            return -1;
        }
        
        io_uring_prep_poll_add(sqe, fd, poll_events);
        struct conn_info ci = { .fd = fd, .type = type };
        memcpy(&sqe->user_data, &ci, sizeof(ci));
        
        // 提交请求
        int ret = io_uring_submit(&_ring);
        if (ret < 0) {
            logError << "io_uring_submit failed: " << ret;
            return ret;
        }
        
        // 保存事件处理器和事件类型
        _mapHander[fd].callback = cb;
        _mapHander[fd].args = args;
        _fd_to_event[fd] = poll_events;
        
        return 0;
    }
    
    async([this, fd, event, cb, args](){
        addEvent(fd, event, cb, args);
    }, true, false);

    return 0;
}

void UringLoop::delEvent(int fd, PollCompleteCB cb) {
    if (!cb) {
        cb = [](bool success) {};
    }

    if (isCurrent()) {
        // 从映射中删除，但io_uring会自动处理完成的请求
        _mapHander.erase(fd);
        _fd_to_event.erase(fd);
        cb(true);
        return ;
    }

    //跨线程操作
    async([this, fd, cb]() {
        delEvent(fd, std::move(const_cast<PollCompleteCB &>(cb)));
    }, true, false);
}

void UringLoop::modifyEvent(int fd, int event, PollCompleteCB cb) {
    if (!cb) {
        cb = [](bool success) {};
    }
    if (isCurrent()) {
        // 先检查fd是否存在
        auto it = _mapHander.find(fd);
        if (it == _mapHander.end()) {
            cb(false);
            return;
        }
        
        // 准备新的poll事件
        uint32_t poll_events = 0;
        int type = 0;
        if (event & EPOLLIN) {
            poll_events |= POLLIN;
            type = EVENT_READ;
        }
        if (event & EPOLLOUT) {
            poll_events |= POLLOUT;
            type = EVENT_WRITE;
        }
        if (event & EPOLLPRI) poll_events |= POLLPRI;
        if (event & EPOLLERR) poll_events |= POLLERR;
        if (event & EPOLLHUP) poll_events |= POLLHUP;
        
        // 添加新的事件监控
        struct io_uring_sqe *sqe = io_uring_get_sqe(&_ring);
        if (!sqe) {
            logError << "io_uring_get_sqe failed";
            cb(false);
            return;
        }
        
        io_uring_prep_poll_add(sqe, fd, poll_events);
        struct conn_info ci = { .fd = fd, .type = type };
        memcpy(&sqe->user_data, &ci, sizeof(ci));
        
        // 提交请求
        int ret = io_uring_submit(&_ring);
        if (ret < 0) {
            logError << "io_uring_submit failed: " << ret;
            cb(false);
            return;
        }
        
        // 更新事件映射
        _fd_to_event[fd] = poll_events;
        
        cb(true);
        return ;
    }
    async([this, fd, event, cb]() {
        modifyEvent(fd, event, std::move(const_cast<PollCompleteCB &>(cb)));
    }, true, false);
}

#endif //ENABLE_URING
