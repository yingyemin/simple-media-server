#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "GB28181SIPConnection.h"
#include "GB28181SIPManager.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"

using namespace std;

GB28181SIPConnection::GB28181SIPConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket)
    :TcpConnection(loop, socket)
    ,_loop(loop)
    ,_socket(socket)
{
    logInfo << "GB28181SIPConnection";
}

GB28181SIPConnection::~GB28181SIPConnection()
{
    logInfo << "~GB28181SIPConnection";
    if (!_deviceId.empty()) {
        GB28181SIPManager::instance()->delContext(_deviceId);
    }
}

void GB28181SIPConnection::init()
{
    weak_ptr<GB28181SIPConnection> wSelf = static_pointer_cast<GB28181SIPConnection>(shared_from_this());
    _parser.setOnGB28181Packet([wSelf](const char* data, int len){
        auto self = wSelf.lock();
        if(!self){
            return;
        }
        shared_ptr<SipRequest> req;
        self->_sipStack.parse_request(req, data, len);
        // auto buffer = StreamBuffer::create();
        // buffer->assign(data + 2, len - 2);
        logTrace << "get a message: " << data;
        self->onSipPacket(self->_socket, req);
    });
}

void GB28181SIPConnection::close()
{
    logInfo << "GB28181SIPConnection::close()";
    _isClose = true;
    TcpConnection::close();
}

void GB28181SIPConnection::onManager()
{
    logInfo << "manager";
}

void GB28181SIPConnection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    //logInfo << "get a buf: " << buffer->size();
    _parser.parse(buffer->data(), buffer->size());
}

void GB28181SIPConnection::onError()
{
    close();
    logWarn << "get a error: ";
}

ssize_t GB28181SIPConnection::send(Buffer::Ptr pkt)
{
    logInfo << "pkt size: " << pkt->size();
    return TcpConnection::send(pkt);
}

void GB28181SIPConnection::onSipPacket(const Socket::Ptr& socket, const SipRequest::Ptr& req)
{
    if (_isClose) {
        return ;
    }
    if (_deviceId.empty()) {
        _deviceId = req->sip_username;
    } else if (_deviceId != req->sip_username) {
        logInfo << _deviceId << ": 收到了其他的包或者tcp丢数据了，ssrc：" << req->sip_username;
        return ;
    }

    if (!_socket) {
        _socket = socket;
    }
    
    if (!_context)
    {
        // string uri = "/live/" + to_string(_ssrc);
        _context = make_shared<GB28181SIPContext>(_loop, _deviceId, DEFAULT_VHOST, PROTOCOL_GB28181, DEFAULT_TYPE);
        if (!_context->init()) {
            _context = nullptr;
            close();
            return ;
        }

        GB28181SIPManager::instance()->addContext(_deviceId, _context);
    } else if (!_context->isAlive()) {
        close();
        return ;
    }

    _context->onSipPacket(socket, req);
}
