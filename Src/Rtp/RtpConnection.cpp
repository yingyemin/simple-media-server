#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "RtpConnection.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

RtpConnection::RtpConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket)
    :TcpConnection(loop, socket)
    ,_loop(loop)
    ,_socket(socket)
{
    logInfo << "RtpConnection";
}

RtpConnection::~RtpConnection()
{
    logInfo << "~RtpConnection";
}

void RtpConnection::init()
{
    weak_ptr<RtpConnection> wSelf = static_pointer_cast<RtpConnection>(shared_from_this());
    
}

void RtpConnection::close()
{
    TcpConnection::close();
}

void RtpConnection::onManager()
{
    logInfo << "manager";
}

void RtpConnection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    // logInfo << "get a buf: " << buffer->size();
}

void RtpConnection::onError()
{
    close();
    logWarn << "get a error: ";
}

ssize_t RtpConnection::send(Buffer::Ptr pkt)
{
    logInfo << "pkt size: " << pkt->size();
    return TcpConnection::send(pkt);
}
