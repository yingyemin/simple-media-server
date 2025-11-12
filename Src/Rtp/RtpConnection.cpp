#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#if defined(_WIN32)
#include "Util/Util.h"
#else
#include <arpa/inet.h>
#endif
#include "RtpConnection.h"
#include "RtpManager.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "Common/Define.h"

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
    if (_ssrc >= 0) {
        RtpManager::instance()->delContext(_ssrc);
    }
}

void RtpConnection::init()
{
    weak_ptr<RtpConnection> wSelf = static_pointer_cast<RtpConnection>(shared_from_this());
    _parser.setOnRtpPacket([wSelf](const StreamBuffer::Ptr& buffer){
        auto self = wSelf.lock();
        if(!self){
            return;
        }
        if (buffer->size() < 12) {
            logError << "rtp packet size too small:" << buffer->size();
            return;
        }
        // auto buffer = StreamBuffer::create();
        // buffer->assign(data + 2, len - 2);
        RtpPacket::Ptr rtp = make_shared<RtpPacket>(buffer, 0);
        self->onRtpPacket(rtp);
    });
}

void RtpConnection::close()
{
    logInfo << "RtpConnection::close()";
    _isClose = true;
    TcpConnection::close();
}

void RtpConnection::onManager()
{
    logInfo << "manager";
}

void RtpConnection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    //logInfo << "get a buf: " << buffer->size();
    _parser.parse(buffer->data(), buffer->size());
}

void RtpConnection::onError(const std::string& errMsg)
{
    close();
    logWarn << "get a error: " << errMsg;
}

ssize_t RtpConnection::send(Buffer::Ptr pkt)
{
    logInfo << "pkt size: " << pkt->size();
    return TcpConnection::send(pkt);
}

void RtpConnection::onRtpPacket(const RtpPacket::Ptr& rtp)
{
    if (_isClose) {
        return ;
    }

    if (rtp->getHeader()->version != 2) {
        // rtp version必须是2.否则，可能是tcp流错位了，或者rtp包有问题
        onError("rtp version must be 2");
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
        _context = make_shared<RtpContext>(_loop, uri, DEFAULT_VHOST, PROTOCOL_RTP, DEFAULT_TYPE);
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
