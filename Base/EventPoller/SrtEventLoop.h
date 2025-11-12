#ifndef SrtEventLoop_h
#define SrtEventLoop_h

#include <mutex>
#include <list>
#include <thread>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <iostream>

#include "Timer.h"
#include "EventLoop.h"

// using namespace std;

#ifdef ENABLE_SRT

class SrtEventHander {
public:
    using eventCallback = std::function<void(int event, void* args)>;
    eventCallback callback;
    void* args;
};

class SrtEventLoop : public EventLoop {
public:
    using Ptr = std::shared_ptr<SrtEventLoop>;
    using asyncEventFunc = std::function<void()>;
    using PollCompleteCB = std::function<void(bool success)>;
    using TaskCompleteCB = std::function<void (bool success, std::shared_ptr<TimerTask>)>;

    SrtEventLoop();
    ~SrtEventLoop();

public:
    static SrtEventLoop::Ptr getCurrentLoop();

public:
    void start() override;
    void setThread(std::thread* thd) override;

    bool isCurrent() override;
    void onAsyncEvent() override;
    void async(asyncEventFunc func, bool sync, bool front = false) override;

    void addTimerTask(uint64_t ms, const TimerTask::timerHander &handler, TaskCompleteCB cb) override;

    int addEvent(int fd, int event, SrtEventHander::eventCallback cb, void* args = nullptr) override;
    void delEvent(int fd, PollCompleteCB cb) override;
    void modifyEvent(int fd, int event, PollCompleteCB cb) override;

    void computeLoad() override;
    void getLoad(int& lastWaitDuration, int& lastRunDuration, int& curWaitDuration, int& curRunDuration) override;

    int getEpollFd()  override {return _epollFd;}
    int getFdCount()  override {return _fdCount;}
    int getTimerTaskCount()  override {return _timerTaskCount;}

private:
    bool _quit =false;
    int _epollFd = -1;
    int _wakeupFd = -1;
    
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
    std::unordered_map<int, SrtEventHander> _mapHander;
};

#endif

#endif //EventLoop_h