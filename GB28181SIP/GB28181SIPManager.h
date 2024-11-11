#ifndef GB28181SIPManager_h
#define GB28181SIPManager_h


#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "GB28181SIPContext.h"
#include "Net/Socket.h"

using namespace std;

class GB28181SIPManager : public enable_shared_from_this<GB28181SIPManager> {
public:
    using Ptr = shared_ptr<GB28181SIPManager>;
    using Wptr = weak_ptr<GB28181SIPManager>;

    GB28181SIPManager();
    ~GB28181SIPManager();

public:
    static GB28181SIPManager::Ptr& instance();

    void init(const EventLoop::Ptr& loop);
    void onSipPacket(const Socket::Ptr& socket, const StreamBuffer::Ptr& req, struct sockaddr* addr, int len);
    void heartbeat();
    void addContext(const string& deviceId, const GB28181SIPContext::Ptr& context);
    void delContext(const string& deviceId);

private:
    bool _isInited = false;
    mutex _contextLck;
    SipStack _sipStack;
    // EventLoop::Ptr _loop;
    unordered_map<string, GB28181SIPContext::Ptr> _mapContext;
};

#endif //GB28181SIPManager_h