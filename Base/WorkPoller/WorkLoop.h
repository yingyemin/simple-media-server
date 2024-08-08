#ifndef WorkLoop_h
#define WorkLoop_h

#include <deque>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <thread>

class WorkTask
{
public:
    using Ptr = std::shared_ptr<WorkTask>;

    int priority_;
    std::function<void()> func_;
};

struct LessByPriority {
    bool operator()(const WorkTask::Ptr& lhs, const WorkTask::Ptr& rhs) const {
        return lhs->priority_ > rhs->priority_;
    }
};

class WorkLoop : public std::enable_shared_from_this<WorkLoop> {
public:
    using Ptr = std::shared_ptr<WorkLoop>;
    WorkLoop(int i);
    ~WorkLoop();

public:
    void startForOrder();
    void setOrderThread(std::thread* thd);
    void addOrderTask(const WorkTask::Ptr& task);
    void notify();

    void start();
    void setThread(std::thread* thd);
    static void addTask(const WorkTask::Ptr& task);
    static WorkTask::Ptr getTask();
    static void notifyOne();
    static void notifyAll();

protected:
    static std::mutex _mtx;
    static std::condition_variable _cv;
    static std::deque<WorkTask::Ptr> _taskList;
    
    bool _stop = false;
    int _index;
    std::thread* _loopThread = nullptr;
    std::thread* _loopOrderThread = nullptr;
    std::mutex _mtxOrder;
    std::condition_variable _cvOrder;
    std::deque<WorkTask::Ptr> _taskOrderList;
    // std::priority_queue<WorkTask::Ptr, vector<WorkTask::Ptr>, LessByPriority> _taskList;
};


#endif //WorkLoop_h