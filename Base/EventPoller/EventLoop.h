#ifndef EventLoop_h
#define EventLoop_h

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

using namespace std;
using namespace moodycamel;

class EventHander {
public:
    using eventCallback = function<void(int event, void* args)>;
    eventCallback callback;
    void* args;
};

class EventLoop : public std::enable_shared_from_this<EventLoop> {
public:
    using Ptr = std::shared_ptr<EventLoop>;
    using asyncEventFunc = function<void()>;
    using PollCompleteCB = std::function<void(bool success)>;
    using TaskCompleteCB = std::function<void (bool success, shared_ptr<TimerTask>)>;

    EventLoop();
    ~EventLoop();

public:
    static EventLoop::Ptr getCurrentLoop();

public:
    virtual void start();
    virtual void setThread(thread* thd);

    virtual bool isCurrent();
    virtual void onAsyncEvent();
    virtual void async(asyncEventFunc func, bool sync, bool front = false);

    virtual void addTimerTask(uint64_t ms, const TimerTask::timerHander &handler, TaskCompleteCB cb);

    virtual int addEvent(int fd, int event, EventHander::eventCallback cb, void* args = nullptr);
    virtual void delEvent(int fd, PollCompleteCB cb);
    virtual void modifyEvent(int fd, int event, PollCompleteCB cb);

    virtual void computeLoad();
    virtual void getLoad(int& lastWaitDuration, int& lastRunDuration, int& curWaitDuration, int& curRunDuration);

    virtual int getEpollFd() {return _epollFd;}
    virtual int getFdCount() {return _fdCount;}
    virtual int getTimerTaskCount() {return _timerTaskCount;}

    virtual void setEpollID(int id) {_epollID = id;}
    virtual int getEpollID() {return _epollID;}

private:
    bool _quit =false;
    bool _eventRun = false;
    int _epollFd = -1;
    int _wakeupFd = -1;
    int _epollID = -1;
    
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
    unordered_map<int, EventHander> _mapHander;

    std::vector<shared_ptr<ReaderWriterQueue<asyncEventFunc>>> _asyncQueues;
};

#endif //EventLoop_h