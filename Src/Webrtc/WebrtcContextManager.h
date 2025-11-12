#ifndef WebrtcContextManager_h
#define WebrtcContextManager_h


#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "WebrtcContext.h"
#include "EventPoller/EventLoop.h"
// #include "WebRtcRtpPacket.h"

// using namespace std;

class WebrtcContextManager : public std::enable_shared_from_this<WebrtcContextManager> {
public:
    using Ptr = std::shared_ptr<WebrtcContextManager>;
    using Wptr = std::weak_ptr<WebrtcContextManager>;

    WebrtcContextManager();
    ~WebrtcContextManager();

public:
    static WebrtcContextManager::Ptr& instance();

    void init(const EventLoop::Ptr& loop);
    void onUdpPacket(const Socket::Ptr& socket, const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len);
    void heartbeat();
    void addContext(const std::string& key, const WebrtcContext::Ptr& context);
    WebrtcContext::Ptr getContext(const std::string& key);
    void delContext(const std::string& key);
    void delContextByHash(const std::string& hash);
    WebrtcContext::Ptr getContextByHash(const std::string& hash);

private:
    void onStunPacket(const Socket::Ptr& socket, const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len);
    void onDtlsPacket(const Socket::Ptr& socket, const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len);
    void onRtcpPacket(const Socket::Ptr& socket, const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len);
    void onRtpPacket(const Socket::Ptr& socket, const RtpPacket::Ptr& rtp, struct sockaddr* addr, int len);

private:
    bool _isInited = false;
    std::mutex _contextLck;
    // EventLoop::Ptr _loop;
    std::unordered_map<std::string, WebrtcContext::Ptr> _mapContext;

    std::mutex _addrToContextLck;
    std::unordered_map<std::string, WebrtcContext::Ptr> _mapAddrToContext;
};

#endif //GB28181Manager_h