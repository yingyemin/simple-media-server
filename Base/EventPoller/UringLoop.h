#ifndef UringLoop_h
#define UringLoop_h

#if defined(ENABLE_URING)

#include <mutex>
#include <list>
#include <thread>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <iostream>
#include <vector>

#include "Timer.h"
#include "ReadWriteQueue/atomicops.h"
#include "ReadWriteQueue/readerwriterqueue.h"
#include "EventLoop.h"

// 添加io_uring头文件
#include <liburing.h>

// using namespace std;
using namespace moodycamel;

class UringLoop : public EventLoop {
public:
    using Ptr = std::shared_ptr<UringLoop>;
    using asyncEventFunc = std::function<void()>;
    using PollCompleteCB = std::function<void(bool success)>;
    using TaskCompleteCB = std::function<void (bool success, std::shared_ptr<TimerTask>)>;

    UringLoop();
    ~UringLoop();

public:
    static UringLoop::Ptr getCurrentLoop();

public:
    virtual void start();
    virtual void setThread(std::thread* thd);

    virtual bool isCurrent();
    virtual void onAsyncEvent();
    virtual void async(asyncEventFunc func, bool sync, bool front = false);

    virtual void addTimerTask(uint64_t ms, const TimerTask::timerHander &handler, TaskCompleteCB cb);

    virtual int addEvent(int fd, int event, EventHander::eventCallback cb, void* args = nullptr);
    virtual void delEvent(int fd, PollCompleteCB cb);
    virtual void modifyEvent(int fd, int event, PollCompleteCB cb);

    virtual void computeLoad();
    virtual void getLoad(int& lastWaitDuration, int& lastRunDuration, int& curWaitDuration, int& curRunDuration);

    virtual int getEpollFd() {return _io_uring_fd;}
    virtual int getFdCount() {return _fdCount;}
    virtual int getTimerTaskCount() {return _timerTaskCount;}

    virtual void setEpollID(int id) {_epollID = id;}
    virtual int getEpollID() {return _epollID;}

private:
    // io_uring相关
    struct io_uring _ring;
    int _io_uring_fd = -1; // 用于兼容旧接口
    int _wakeupFd = -1;
    int _epollID = -1;
    
    // 事件循环状态
    bool _quit = false;
    bool _eventRun = false;
    int _fdCount = 0;
    int _timerTaskCount = 0;
    uint64_t _waitTime = 0;
    uint64_t _runTime = 0;
    uint64_t _lastWaitDuration = 0;
    uint64_t _lastRunDuration = 0;
    uint64_t _curWaitDuration = 0;
    uint64_t _curRunDuration = 0;

    uint64_t _delayTaskDuration = 0;
    uint64_t _eventDuration = 0;
    uint64_t _asyncEventDuration = 0;

    std::thread* _loopThread = nullptr;
    std::mutex _mtxEvents;
    Timer::Ptr _timer;
    std::list<asyncEventFunc> _asyncEvents;
    std::unordered_map<int, EventHander> _mapHander;
    std::unordered_map<int, uint32_t> _fd_to_event;

    std::vector<std::shared_ptr<ReaderWriterQueue<asyncEventFunc>>> _asyncQueues;

    // 创建唤醒fd
    static int createEventfd();
    // 提交io_uring请求
    void submit_io_uring_cqe();
};

#endif //ENABLE_URING

#endif //UringLoop_h
