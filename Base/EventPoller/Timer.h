#ifndef Timer_h
#define Timer_h

#include <map>
#include <memory>
#include <functional>

using namespace std;


class TimerTask
{
public:
    TimerTask();
    ~TimerTask();

public:
    using timerHander = function<uint64_t ()>;
    bool quit = false;
    uint64_t delay;
    uint64_t click;
    timerHander hander;
};

struct CmpByTimerTask {
    bool operator()(const shared_ptr<TimerTask> k1, const shared_ptr<TimerTask> k2) {
        return (k1)->click < (k2)->click;
    }
};

class Timer : public std::enable_shared_from_this<Timer> {
public:
    using Ptr = shared_ptr<Timer>;
    Timer();
    ~Timer() = default;

public:
    shared_ptr<TimerTask> addTimer(uint64_t ms, const TimerTask::timerHander &cb);
    void delTimer(const shared_ptr<TimerTask> &task);
    uint64_t flushTimerTask();
    int getTaskSize();

public:

private:
    std::multimap<shared_ptr<TimerTask>, shared_ptr<TimerTask>, CmpByTimerTask> _tasks;
};

#endif //Timer_h