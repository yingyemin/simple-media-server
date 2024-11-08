#ifndef GB28181SIPConnection_H
#define GB28181SIPConnection_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Net/TcpConnection.h"
#include "GB28181SIPParser.h"
#include "GB28181SIPContext.h"


using namespace std;

class GB28181SIPConnection : public TcpConnection
{
public:
    using Ptr = shared_ptr<GB28181SIPConnection>;
    using Wptr = weak_ptr<GB28181SIPConnection>;

    GB28181SIPConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket);
    ~GB28181SIPConnection();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError() override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;

    void onSipPacket(const SipRequest::Ptr& req);

private:
    bool _isClose = false;
    string _deviceId;
    GB28181SIPParser _parser;
    SipStack _sipStack;
    GB28181SIPContext::Ptr _context;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
};


#endif //GB28181SIPConnection_H
