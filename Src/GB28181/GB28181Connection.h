#ifndef GB28181Connection_H
#define GB28181Connection_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Net/TcpConnection.h"
#include "Rtp/RtpPacket.h"
#include "GB28181Parser.h"
#include "GB28181Context.h"


// using namespace std;

class GB28181Connection : public TcpConnection
{
public:
    using Ptr = std::shared_ptr<GB28181Connection>;
    using Wptr = std::weak_ptr<GB28181Connection>;

    GB28181Connection(const EventLoop::Ptr& loop, const Socket::Ptr& socket);
    ~GB28181Connection();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError(const std::string& msg) override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;

    void onRtpPacket(const RtpPacket::Ptr& rtp);

private:
    bool _isClose = false;
    int64_t _ssrc = -1;
    GB28181Parser _parser;
    GB28181Context::Ptr _context;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
};


#endif //GB28181Connection_H
