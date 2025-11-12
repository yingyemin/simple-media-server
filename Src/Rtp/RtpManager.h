#ifndef RtpManager_h
#define RtpManager_h


#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "RtpContext.h"

// using namespace std;

class RtpManager : public std::enable_shared_from_this<RtpManager> {
public:
    using Ptr = std::shared_ptr<RtpManager>;
    using Wptr = std::weak_ptr<RtpManager>;

    RtpManager();
    ~RtpManager();

public:
    static RtpManager::Ptr& instance();

    void init(const EventLoop::Ptr& loop);
    void onRtpPacket(const RtpPacket::Ptr& rtp, struct sockaddr* addr, int len);
    void heartbeat();
    void addContext(uint32_t ssrc, const RtpContext::Ptr& context);
    void delContext(uint32_t ssrc);

private:
    bool _isInited = false;
    std::mutex _contextLck;
    // EventLoop::Ptr _loop;
    std::unordered_map<uint32_t, RtpContext::Ptr> _mapContext;
};

#endif //RtpManager_h