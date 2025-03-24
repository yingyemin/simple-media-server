#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "Ehome2Connection.h"
#include "Rtp/RtpManager.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"

using namespace std;

Ehome2Connection::Ehome2Connection(const EventLoop::Ptr& loop, const Socket::Ptr& socket)
    :TcpConnection(loop, socket)
    ,_loop(loop)
    ,_socket(socket)
{
    logInfo << "Ehome2Connection";
}

Ehome2Connection::~Ehome2Connection()
{
    logInfo << "~Ehome2Connection";
    if (_ssrc >= 0) {
        RtpManager::instance()->delContext(_ssrc);
    }
}

void Ehome2Connection::init()
{
    weak_ptr<Ehome2Connection> wSelf = static_pointer_cast<Ehome2Connection>(shared_from_this());
    _parser.setOnRtpPacket([wSelf](const char* data, int len){
        auto self = wSelf.lock();
        if(!self){
            return;
        }
        auto buffer = StreamBuffer::create();
        buffer->assign(data + 4, len - 4);
        RtpPacket::Ptr rtp = make_shared<RtpPacket>(buffer, 0);
        self->onRtpPacket(rtp);
    });
}

void Ehome2Connection::close()
{
    logInfo << "Ehome2Connection::close()";
    TcpConnection::close();
}

void Ehome2Connection::onManager()
{
    logInfo << "manager";
}

void Ehome2Connection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    // logInfo << "get a buf: " << buffer->size();
    _parser.parse(buffer->data(), buffer->size());
}

void Ehome2Connection::onError()
{
    close();
    logWarn << "get a error: ";
}

ssize_t Ehome2Connection::send(Buffer::Ptr pkt)
{
    logInfo << "pkt size: " << pkt->size();
    return TcpConnection::send(pkt);
}

void Ehome2Connection::onRtpPacket(const RtpPacket::Ptr& rtp)
{
    if (_ssrc < 0) {
        _ssrc = rtp->getSSRC();
    } else if (_ssrc != rtp->getSSRC()) {
        logInfo << "收到了其他的包或者tcp丢数据了，ssrc：" << rtp->getSSRC();
        return ;
    }
    
    if (!_context)
    {
        string uri = "/live/" + to_string(_ssrc);
        _context = make_shared<RtpContext>(_loop, uri, DEFAULT_VHOST, PROTOCOL_EHOME2, DEFAULT_TYPE);
        if (!_context->init()) {
            _context = nullptr;
            close();
            return ;
        }

        RtpManager::instance()->addContext(_ssrc, _context);
    } else if (!_context->isAlive()) {
        close();
        return ;
    }

    _context->onRtpPacket(rtp);
}
