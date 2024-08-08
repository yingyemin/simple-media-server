/*
 * Copyright (c) 2016 The ZLToolKit project authors. All Rights Reserved.
 *
 * This file is part of ZLToolKit(https://github.com/ZLMediaKit/ZLToolKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "WorkLoopPool.h"
#include "Log/Logger.h"

using namespace std;

WorkLoopPool::WorkLoopPool()
{}

std::shared_ptr<WorkLoopPool> WorkLoopPool::instance() 
{ 
    static std::shared_ptr<WorkLoopPool> pool(new WorkLoopPool()); 
    return pool; 
}

void WorkLoopPool::init(int size, int priority, bool affinity)
{
    auto cpus = thread::hardware_concurrency();
    size = size > 0 ? size : cpus;
    _threadSize = size;

    for (int i = 0; i < size; ++i) {
        auto loopName = "WorkLoop " + to_string(i);
        logInfo << loopName;
        auto cpuIndex = i % cpus;
        WorkLoop::Ptr loop(new WorkLoop(i));
        std::thread* loopThd = new std::thread(&WorkLoop::start, loop);
        loop->setThread(loopThd);
        
        std::thread* loopOrderThd = new std::thread(&WorkLoop::startForOrder, loop);
        loop->setOrderThread(loopOrderThd);
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

void WorkLoopPool::for_each_loop(const function<void(const WorkLoop::Ptr &)> &cb)
{
    for (auto& loop : _loops) {
        cb(loop);
    }
}

WorkLoop::Ptr WorkLoopPool::getLoopByCircle()
{
    return _loops[_index++ % _threadSize];
}

WorkLoop::Ptr WorkLoopPool::getLoop(int loopIndex)
{
    if (loopIndex > _threadSize) {
        logWarn << "loop index > work thread size";
        return nullptr;
    }
    return _loops[loopIndex];
}

void WorkLoopPool::addTask(const WorkTask::Ptr& task)
{
    WorkLoop::addTask(task);
}

WorkTask::Ptr WorkLoopPool::getTask()
{
    return WorkLoop::getTask();;
}
