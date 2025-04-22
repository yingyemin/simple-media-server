#ifndef WebrtcConnection_H
#define WebrtcConnection_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Net/TcpConnection.h"
#include "Rtp/RtpPacket.h"
#include "WebrtcParser.h"
#include "WebrtcContext.h"


using namespace std;

class WebrtcConnection : public TcpConnection
{
public:
    using Ptr = shared_ptr<WebrtcConnection>;
    using Wptr = weak_ptr<WebrtcConnection>;

    WebrtcConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket);
    ~WebrtcConnection();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError(const string& msg) override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;

    void onRtcPacket(const char* data, int len);
    void onStunPacket(const StreamBuffer::Ptr& buffer);

private:
    int _ssrc = -1;
    string _username;
    WebrtcParser _parser;
    WebrtcContext::Ptr _context;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
};


#endif //GB28181Connection_H
