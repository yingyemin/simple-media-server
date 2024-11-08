#ifndef GB28181SIPContext_H
#define GB28181SIPContext_H

#include "Buffer.h"
#include "Util/TimeClock.h"
#include "EventPoller/EventLoop.h"
#include "SipMessage.h"

#include <memory>
#include <unordered_map>

using namespace std;

class GB28181SIPContext : public enable_shared_from_this<GB28181SIPContext>
{
public:
    using Ptr = shared_ptr<GB28181SIPContext>;
    using Wptr = weak_ptr<GB28181SIPContext>;

    GB28181SIPContext(const EventLoop::Ptr& loop, const string& deviceId, const string& vhost, const string& protocol, const string& type);
    ~GB28181SIPContext();
public:
    bool init();
    void onSipPacket(const SipRequest::Ptr& req, struct sockaddr* addr = nullptr, int len = 0, bool sort = false);
    void heartbeat();
    bool isAlive() {return _alive;}

private:
    bool _alive = true;
    string _deviceId;
    string _vhost;
    string _protocol;
    string _type;
    string _payloadType = "ps";
    TimeClock _timeClock;
    shared_ptr<sockaddr> _addr;
    EventLoop::Ptr _loop;
};



#endif //GB28181SIPContext_H
