#ifndef Ehome2Connection_H
#define Ehome2Connection_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Net/TcpConnection.h"
#include "Rtp/RtpPacket.h"
#include "Ehome2Parser.h"
#include "GB28181/GB28181Context.h"


using namespace std;

class Ehome2Connection : public TcpConnection
{
public:
    using Ptr = shared_ptr<Ehome2Connection>;
    using Wptr = weak_ptr<Ehome2Connection>;

    Ehome2Connection(const EventLoop::Ptr& loop, const Socket::Ptr& socket);
    ~Ehome2Connection();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError() override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;

    void onRtpPacket(const RtpPacket::Ptr& rtp);

private:
    int _ssrc = -1;
    Ehome2Parser _parser;
    GB28181Context::Ptr _context;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
};


#endif //Ehome2Connection_H
