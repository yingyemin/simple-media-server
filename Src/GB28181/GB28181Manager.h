#ifndef GB28181Manager_h
#define GB28181Manager_h


#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "GB28181Context.h"

using namespace std;

class GB28181Manager : public enable_shared_from_this<GB28181Manager> {
public:
    using Ptr = shared_ptr<GB28181Manager>;
    using Wptr = weak_ptr<GB28181Manager>;

    GB28181Manager();
    ~GB28181Manager();

public:
    static GB28181Manager::Ptr& instance();

    void init(const EventLoop::Ptr& loop);
    void onRtpPacket(const RtpPacket::Ptr& rtp, struct sockaddr* addr, int len);
    void heartbeat();
    void addContext(uint32_t ssrc, const GB28181Context::Ptr& context);
    void delContext(uint32_t ssrc);

private:
    bool _isInited = false;
    mutex _contextLck;
    // EventLoop::Ptr _loop;
    unordered_map<uint32_t, GB28181Context::Ptr> _mapContext;
};

#endif //GB28181Manager_h