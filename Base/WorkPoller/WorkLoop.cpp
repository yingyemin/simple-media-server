/*
 * Copyright (c) 2016 The ZLToolKit project authors. All Rights Reserved.
 *
 * This file is part of ZLToolKit(https://github.com/ZLMediaKit/ZLToolKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "WorkLoop.h"
#include "WorkLoopPool.h"
#include "Log/Logger.h"
#include "Util/Thread.h"

using namespace std;

mutex WorkLoop::_mtx;
condition_variable WorkLoop::_cv;
deque<WorkTask::Ptr> WorkLoop::_taskList;

WorkLoop::WorkLoop(int i)
    :_index(i)
{}

WorkLoop::~WorkLoop()
{}

void WorkLoop::start()
{
    Thread::setThreadName("worker-" + to_string(_index));
    while (true) {
        WorkTask::Ptr task;
        {
            unique_lock<mutex> lock(_mtx);
            _cv.wait(lock, [this]() { return !_taskList.empty() || _stop; });

            if (_stop) {
                break;
            }

            if (_taskList.empty()) {
                continue;
            } else {
                task = _taskList.front();
                if (!task) {
                    continue;
                }
                _taskList.pop_front();
            }
        }
        task->func_(); // Execute the task.
    }
}

void WorkLoop::addTask(const WorkTask::Ptr& task)
{
    {
        lock_guard<mutex> lck(_mtx);
        _taskList.emplace_back(task);
    }

    notifyOne();
}

WorkTask::Ptr WorkLoop::getTask()
{
    lock_guard<mutex> lck(_mtx);
    if (_taskList.empty()) {
        return nullptr;
    }

    auto task = _taskList.front();
    _taskList.pop_front();

    return task;
}

void WorkLoop::notifyOne()
{
    _cv.notify_one();
}

void WorkLoop::notifyAll()
{
    _cv.notify_all();
}

void WorkLoop::setThread(thread* thd)
{
    _loopThread = thd;
}

void WorkLoop::startForOrder()
{
    Thread::setThreadName("worker-order-" + to_string(_index));
    while (true) {
        WorkTask::Ptr task;
        {
            logInfo << "waiting new task";
            unique_lock<mutex> lock(_mtxOrder);
            _cvOrder.wait(lock, [this]() { return !_taskOrderList.empty() || _stop; });

            logInfo << "get a task";
            if (_stop) {
                break;
            }

            if (_taskOrderList.empty()) {
                continue;
            } else {
                task = _taskOrderList.front();
                if (!task) {
                    continue;
                }
                _taskOrderList.pop_front();
            }
        }
        logInfo << "do a task";
        task->func_(); // Execute the task.
    }
}

void WorkLoop::setOrderThread(thread* thd)
{
    _loopOrderThread = thd;
}

void WorkLoop::addOrderTask(const WorkTask::Ptr& task)
{
    logInfo << "add a task";
    {
        lock_guard<mutex> lck(_mtxOrder);
        _taskOrderList.emplace_back(task);
    }

    notify();
}

void WorkLoop::notify()
{
    _cvOrder.notify_one();
}