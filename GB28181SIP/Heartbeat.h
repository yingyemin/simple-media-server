#ifndef GB28181_Heartbeat_H
#define GB28181_Heartbeat_H

#include <string>
#include <memory>

#include "EventPoller/EventLoop.h"
#include "Common/HookManager.h"

// using namespace std;

class Heartbeat : public std::enable_shared_from_this<Heartbeat>
{
public:
    using Ptr = std::shared_ptr<Heartbeat>;

    Heartbeat();
    ~Heartbeat();

    void startAsync();
    void start();
    void getDeviceInfo(ServerInfo& info);
    void registerServer();

public:

private:
    EventLoop::Ptr _loop;
};


#endif //Heartbeat_H
