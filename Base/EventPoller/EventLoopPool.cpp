/*
 * Copyright (c) 2016 The ZLToolKit project authors. All Rights Reserved.
 *
 * This file is part of ZLToolKit(https://github.com/ZLMediaKit/ZLToolKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "EventLoopPool.h"
#include "Log/Logger.h"

using namespace std;

EventLoopPool::EventLoopPool()
{}

std::shared_ptr<EventLoopPool> EventLoopPool::instance() 
{ 
    static std::shared_ptr<EventLoopPool> pool(new EventLoopPool()); 
    return pool; 
}

void EventLoopPool::init(int size, int priority, bool affinity)
{
    auto cpus = thread::hardware_concurrency();
    size = size > 0 ? size : cpus;
    _threadSize = size;

    for (int i = 0; i < size; ++i) {
        auto loopName = "EventLoop " + to_string(i);
        logInfo << loopName;
        auto cpuIndex = i % cpus;
        EventLoop::Ptr loop(new EventLoop());
        std::thread* loopThd = new std::thread(&EventLoop::start, loop);
        loop->setThread(loopThd);
        // loop->async([cpuIndex, loopName, priority, affinity]() {
        //     // 设置线程优先级
        //     ThreadPool::setPriority((ThreadPool::Priority)priority);
        //     // 设置线程名
        //     setThreadName(full_name.data());
        //     // 设置cpu亲和性
        //     if (affinity) {
        //         setThreadAffinity(cpu_index);
        //     }
        // });
        _loops.emplace_back(std::move(loop));
    }
}

void EventLoopPool::for_each_loop(const function<void(const EventLoop::Ptr &)> &cb)
{
    for (auto& loop : _loops) {
        cb(loop);
    }
}

EventLoop::Ptr EventLoopPool::getLoopByCircle()
{
    return _loops[_index++ % _threadSize];
}


//////////////////////////////////////////////////////////////

SrtEventLoopPool::SrtEventLoopPool()
{}

std::shared_ptr<SrtEventLoopPool> SrtEventLoopPool::instance() 
{ 
    static std::shared_ptr<SrtEventLoopPool> pool(new SrtEventLoopPool()); 
    return pool; 
}

void SrtEventLoopPool::init(int size, int priority, bool affinity)
{
    auto cpus = thread::hardware_concurrency();
    size = size > 0 ? size : cpus;
    _threadSize = size;

    for (int i = 0; i < size; ++i) {
        auto loopName = "EventLoop " + to_string(i);
        logInfo << loopName;
        auto cpuIndex = i % cpus;
        SrtEventLoop::Ptr loop(new SrtEventLoop());
        std::thread* loopThd = new std::thread(&SrtEventLoop::start, loop);
        loop->setThread(loopThd);
        // loop->async([cpuIndex, loopName, priority, affinity]() {
        //     // 设置线程优先级
        //     ThreadPool::setPriority((ThreadPool::Priority)priority);
        //     // 设置线程名
        //     setThreadName(full_name.data());
        //     // 设置cpu亲和性
        //     if (affinity) {
        //         setThreadAffinity(cpu_index);
        //     }
        // });
        _loops.emplace_back(std::move(loop));
    }
}

void SrtEventLoopPool::for_each_loop(const function<void(const SrtEventLoop::Ptr &)> &cb)
{
    for (auto& loop : _loops) {
        // loop->async([cb, loop](){
            cb(loop);
        // }, true);
    }
}

SrtEventLoop::Ptr SrtEventLoopPool::getLoopByCircle()
{
    return _loops[_index++ % _threadSize];
}
