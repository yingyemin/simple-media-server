#ifndef EventLoopPool_h
#define EventLoopPool_h

#include <vector>
#include "EventLoop.h"
#include "SrtEventLoop.h"


class EventLoopPool : public std::enable_shared_from_this<EventLoopPool> {
public:
    EventLoopPool();
    ~EventLoopPool() = default;

public:
    static std::shared_ptr<EventLoopPool> instance();
    void init(int size, int priority, bool affinity);
    void for_each_loop(const std::function<void(const EventLoop::Ptr &)> &cb, int count = 0);
    EventLoop::Ptr getLoopByCircle();
    int getThreadSize() {return _threadSize;}
    int getStartTime();

protected:
    std::vector<EventLoop::Ptr> _loops;
    int _index = 0;
    int _threadSize = 0;
    int _startTime = 0;
};

#ifdef ENABLE_SRT

class SrtEventLoopPool
{
public:
    SrtEventLoopPool();
    ~SrtEventLoopPool() = default;

    static std::shared_ptr<SrtEventLoopPool> instance();
    void init(int size, int priority, bool affinity);
    void for_each_loop(const std::function<void(const SrtEventLoop::Ptr &)> &cb);
    SrtEventLoop::Ptr getLoopByCircle();

protected:
    std::vector<SrtEventLoop::Ptr> _loops;
    int _index = 0;
    int _threadSize = 0;
};

#endif

#endif //EventLoopPool_h