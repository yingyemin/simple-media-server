#ifndef RtpConnection_H
#define RtpConnection_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Net/TcpConnection.h"
#include "Rtp/RtpPacket.h"
#include "RtpParser.h"
#include "RtpContext.h"


// using namespace std;

class RtpConnection : public TcpConnection
{
public:
    using Ptr = std::shared_ptr<RtpConnection>;
    using Wptr = std::weak_ptr<RtpConnection>;

    RtpConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket);
    ~RtpConnection();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError(const std::string& errMsg) override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;

    void onRtpPacket(const RtpPacket::Ptr& rtp);
    void setUri(const std::string& uri) {_uri = uri; }

private:
    bool _isClose = false;
    int64_t _ssrc = -1;
    std::string _uri;
    RtpParser _parser;
    RtpContext::Ptr _context;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
};


#endif //RtpConnection_H
