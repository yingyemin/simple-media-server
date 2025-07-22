#ifndef SrtManager_h
#define SrtManager_h


#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "SrtContext.h"
#include "Net/Socket.h"

using namespace std;

class SrtManager : public enable_shared_from_this<SrtManager> {
public:
    using Ptr = shared_ptr<SrtManager>;
    using Wptr = weak_ptr<SrtManager>;

    SrtManager();
    ~SrtManager();

public:
    static SrtManager::Ptr& instance();

    void init(const EventLoop::Ptr& loop);
    void onSrtPacket(const Socket::Ptr& socket, const Buffer::Ptr& buffer, struct sockaddr* addr, int len);
    void heartbeat();
    void addContext(uint32_t ssrc, const SrtContext::Ptr& context);
    void delContext(uint32_t ssrc);

private:
    bool _isInited = false;
    mutex _contextLck;
    // EventLoop::Ptr _loop;
    unordered_map<uint32_t, SrtContext::Ptr> _mapContext;
};

#endif //SrtManager_h
