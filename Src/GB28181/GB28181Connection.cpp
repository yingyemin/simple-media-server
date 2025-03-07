#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "GB28181Connection.h"
#include "GB28181Manager.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"

using namespace std;

GB28181Connection::GB28181Connection(const EventLoop::Ptr& loop, const Socket::Ptr& socket)
    :TcpConnection(loop, socket)
    ,_loop(loop)
    ,_socket(socket)
{
    logDebug << "GB28181Connection";
}

GB28181Connection::~GB28181Connection()
{
    logInfo << "~GB28181Connection, ssrc: " << _ssrc;
    if (_ssrc >= 0) {
        GB28181Manager::instance()->delContext(_ssrc);
    }
}

void GB28181Connection::init()
{
    weak_ptr<GB28181Connection> wSelf = static_pointer_cast<GB28181Connection>(shared_from_this());
    _parser.setOnRtpPacket([wSelf](const StreamBuffer::Ptr& buffer){
        auto self = wSelf.lock();
        if(!self){
            return;
        }
        // auto buffer = StreamBuffer::create();
        // buffer->assign(data + 2, len - 2);
        RtpPacket::Ptr rtp = make_shared<RtpPacket>(buffer, 0);
        self->onRtpPacket(rtp);
    });
}

void GB28181Connection::close()
{
    logDebug << "GB28181Connection::close(), ssrc" << _ssrc;
    _isClose = true;
    TcpConnection::close();
}

void GB28181Connection::onManager()
{
    logDebug << "manager, ssrc" << _ssrc;
}

void GB28181Connection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    //logInfo << "get a buf: " << buffer->size();
    _parser.parse(buffer->data(), buffer->size());
}

void GB28181Connection::onError()
{
    close();
    logWarn << "get a error: ; ssrc: " << _ssrc;
}

ssize_t GB28181Connection::send(Buffer::Ptr pkt)
{
    logTrace << "pkt size: " << pkt->size() << ", ssrc" << _ssrc;
    return TcpConnection::send(pkt);
}

void GB28181Connection::onRtpPacket(const RtpPacket::Ptr& rtp)
{
    if (_isClose) {
        return ;
    }

    if (rtp->getHeader()->version != 2) {
        // rtp version必须是2.否则，可能是tcp流错位了，或者rtp包有问题
        onError();
        return ;
    }

    if (_ssrc < 0) {
        _ssrc = rtp->getSSRC();
    } else if (_ssrc != rtp->getSSRC()) {
        logInfo << "收到了其他的包或者tcp丢数据了，ssrc：" << rtp->getSSRC();
        return ;
    }
    
    if (!_context)
    {
        string uri = "/live/" + to_string(_ssrc);
        _context = make_shared<GB28181Context>(_loop, uri, DEFAULT_VHOST, PROTOCOL_GB28181, DEFAULT_TYPE);
        if (!_context->init()) {
            _context = nullptr;
            close();
            return ;
        }

        GB28181Manager::instance()->addContext(_ssrc, _context);
    } else if (!_context->isAlive()) {
        close();
        return ;
    }

    _context->onRtpPacket(rtp);
}
