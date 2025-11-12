#ifndef GB28181SIPManager_h
#define GB28181SIPManager_h


#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "GB28181SIPContext.h"
#include "Net/Socket.h"

// using namespace std;

class GB28181SIPManager : public std::enable_shared_from_this<GB28181SIPManager> {
public:
    using Ptr = std::shared_ptr<GB28181SIPManager>;
    using Wptr = std::weak_ptr<GB28181SIPManager>;

    GB28181SIPManager();
    ~GB28181SIPManager();

public:
    static GB28181SIPManager::Ptr& instance();

    void init(const EventLoop::Ptr& loop);
    void onSipPacket(const Socket::Ptr& socket, const StreamBuffer::Ptr& req, struct sockaddr* addr, int len);
    void heartbeat();
    void addContext(const std::string& deviceId, const GB28181SIPContext::Ptr& context);
    void delContext(const std::string& deviceId);
    GB28181SIPContext::Ptr getContext(const std::string& deviceId);
    std::unordered_map<std::string, GB28181SIPContext::Ptr> getMapContext();

private:
    bool _isInited = false;
    std::mutex _contextLck;
    SipStack _sipStack;
    // EventLoop::Ptr _loop;
    std::unordered_map<std::string, GB28181SIPContext::Ptr> _mapContext;
};

#endif //GB28181SIPManager_h