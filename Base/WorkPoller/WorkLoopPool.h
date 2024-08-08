#ifndef WorkLoopPool_h
#define WorkLoopPool_h

#include <vector>
#include "WorkLoop.h"


class WorkLoopPool : public std::enable_shared_from_this<WorkLoopPool> {
public:
    WorkLoopPool();
    ~WorkLoopPool() = default;

public:
    static std::shared_ptr<WorkLoopPool> instance();
    void init(int size, int priority, bool affinity);
    void for_each_loop(const std::function<void(const WorkLoop::Ptr &)> &cb);
    WorkLoop::Ptr getLoopByCircle();
    WorkLoop::Ptr getLoop(int loopIndex);

    void addTask(const WorkTask::Ptr& task);
    WorkTask::Ptr getTask();

protected:
    int _index = 0;
    int _threadSize = 0;
    std::vector<WorkLoop::Ptr> _loops;
};


#endif //WorkLoopPool_h