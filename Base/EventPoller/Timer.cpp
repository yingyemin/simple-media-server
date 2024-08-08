#include "Timer.h"
#include "Logger.h"
#include <vector>

using namespace std;

static inline uint64_t getTick() {
    uint64_t t;
    struct timespec ti;
    clock_gettime(CLOCK_MONOTONIC, &ti);
    t = (uint64_t)ti.tv_sec * 1000;
    t += ti.tv_nsec / 1000000;
    return t;
}

TimerTask::TimerTask()
{
    logInfo << "TimerTask()";
}

TimerTask::~TimerTask()
{
    logInfo << "~TimerTask(): " << this;
}

Timer::Timer()
{}

shared_ptr<TimerTask> Timer::addTimer(uint64_t ms, const TimerTask::timerHander &cb)
{
    shared_ptr<TimerTask> task = make_shared<TimerTask>();
    task->delay = ms;
    task->click = getTick() + ms;
    task->hander = cb;
    _tasks.emplace(task, task);
    // _tasks[task] = task;

    return task;
}

void Timer::delTimer(const shared_ptr<TimerTask> &task)
{
    logInfo << "delTimer task";
    _tasks.erase(task);
}

int Timer::getTaskSize()
{
    return _tasks.size();
}

uint64_t Timer::flushTimerTask()
{
    uint64_t now = getTick();
    vector<shared_ptr<TimerTask>> tmp;
    // logInfo << _tasks.size();
    while (true) {
        auto it = _tasks.begin(); 
        if (it == _tasks.end())
        {
            break;
        }
        if (it->first->click > now) {
            break;
        }
        // logInfo << "_tasks.erase";
        auto task = it->second;
        // logInfo << task << ", key: " << task;
        _tasks.erase(it);
        // if (_tasks.find(task) != _tasks.end()) {
        //     logInfo << "erase task failed";
        // }
        // logInfo << "task->hander()";
        uint64_t delay = task->hander();
        // logInfo << "(delay > 0 && !task->quit): " << (delay > 0 && !task->quit);
        if (delay > 0 && !task->quit) {
            task->click = now + delay;
            tmp.emplace_back(task);
        }
    }

    // for (auto task : _tasks) {
    //     logInfo << "left task: " << task.first;
    // }

    // logInfo << "add task num: " << tmp.size();
    // logInfo << _tasks.size();
    for (auto& task : tmp) {
        // logInfo << "add task: " << *iter;
        // if (_tasks.find(*iter) != _tasks.end()) {
        //     logInfo << "add task failed";
        // }
        // for (auto task : _tasks) {
        //     logInfo << "left task: " << task.first;
        // }
        // _tasks[*iter] = *iter;
        _tasks.emplace(task, task);
        // for (auto task : _tasks) {
        //     logInfo << "left task: " << task.first;
        // }
    }
    // logInfo << _tasks.size();

    auto it = _tasks.begin();
    if (it == _tasks.end()) {
        return 2000;
    }
    uint64_t delay = it->first->click - now;
    // logInfo << "task delay: " << delay;
    return delay > 0 ? delay : 2000;
}

