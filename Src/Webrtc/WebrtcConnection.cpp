#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "WebrtcConnection.h"
#include "WebrtcContextManager.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"
#include "Webrtc.h"

using namespace std;

WebrtcConnection::WebrtcConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket)
    :TcpConnection(loop, socket)
    ,_loop(loop)
    ,_socket(socket)
{
    logInfo << "WebrtcConnection";
}

WebrtcConnection::~WebrtcConnection()
{
    logInfo << "~WebrtcConnection";
    WebrtcContextManager::instance()->delContext(_username);
}

void WebrtcConnection::init()
{
    weak_ptr<WebrtcConnection> wSelf = static_pointer_cast<WebrtcConnection>(shared_from_this());
    _parser.setOnRtcPacket([wSelf](const char* data, int len){
        auto self = wSelf.lock();
        if(!self){
            return;
        }
        
        self->onRtcPacket(data + 2, len - 2);
    });
}

void WebrtcConnection::close()
{
    logInfo << "WebrtcConnection::close()";
    TcpConnection::close();
}

void WebrtcConnection::onManager()
{
    logInfo << "manager";
}

void WebrtcConnection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    //logInfo << "get a buf: " << buffer->size();
    _parser.parse(buffer->data(), buffer->size());
}

void WebrtcConnection::onError()
{
    close();
    logWarn << "get a error: ";
}

ssize_t WebrtcConnection::send(Buffer::Ptr pkt)
{
    logInfo << "pkt size: " << pkt->size();
    return TcpConnection::send(pkt);
}

void WebrtcConnection::onRtcPacket(const char* data, int len)
{
    auto buffer = StreamBuffer::create();
    buffer->assign(data, len);

    int pktType = guessType(buffer);
    if (pktType != kStunPkt && !_context) {
        logWarn << "rtc context is empty";
        return ;
    }
    switch(pktType) {
        case kStunPkt: {
            onStunPacket(buffer);
            break;
        }
        case kDtlsPkt: {
            _context->onDtlsPacket(_socket, buffer, nullptr, 0);
            break;
        }
        case kRtpPkt: {
            auto rtp = make_shared<WebrtcRtpPacket>(buffer, 0);
            _context->onRtpPacket(_socket, rtp, nullptr, 0);
            break;
        }
        case kRtcpPkt: {
            _context->onRtcpPacket(_socket, buffer, nullptr, 0);
            break;
        }
        default: {
            logWarn << "unknown pkt type: " << pktType;
            break;
        }
    }
}

void WebrtcConnection::onStunPacket(const StreamBuffer::Ptr& buffer)
{
	WebrtcStun stunReq;
	if (0 != stunReq.parse(buffer->data(), buffer->size())) {
		logError << "parse stun packet failed";
		return;
	}

    logInfo << "get peer addr";
    sockaddr * addr;
    if (_socket->getFamily() == AF_INET) {
        addr = (sockaddr *)_socket->getPeerAddr4();
    } else {
        addr = (sockaddr *)_socket->getPeerAddr6();
    }

	_username = stunReq.getUsername();
	auto context = WebrtcContextManager::instance()->getContext(_username);

	if (context) {
        context->onStunPacket(_socket, stunReq, addr, sizeof(sockaddr));
        _context = context;
	}
}