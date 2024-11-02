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
    void for_each_loop(const function<void(const EventLoop::Ptr &)> &cb);
    EventLoop::Ptr getLoopByCircle();

protected:
    std::vector<EventLoop::Ptr> _loops;
    int _index = 0;
    int _threadSize = 0;
};

#ifdef ENABLE_SRT

class SrtEventLoopPool
{
public:
    SrtEventLoopPool();
    ~SrtEventLoopPool() = default;

    static std::shared_ptr<SrtEventLoopPool> instance();
    void init(int size, int priority, bool affinity);
    void for_each_loop(const function<void(const SrtEventLoop::Ptr &)> &cb);
    SrtEventLoop::Ptr getLoopByCircle();

protected:
    std::vector<SrtEventLoop::Ptr> _loops;
    int _index = 0;
    int _threadSize = 0;
};

#endif

#endif //EventLoopPool_h