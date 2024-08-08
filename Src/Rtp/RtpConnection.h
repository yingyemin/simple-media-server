#ifndef RtpConnection_H
#define RtpConnection_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Net/TcpConnection.h"


using namespace std;

class RtpConnection : public TcpConnection
{
public:
    using Ptr = shared_ptr<RtpConnection>;
    using Wptr = weak_ptr<RtpConnection>;

    RtpConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket);
    ~RtpConnection();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError() override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;

private:
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
};


#endif //RtpConnection_H
