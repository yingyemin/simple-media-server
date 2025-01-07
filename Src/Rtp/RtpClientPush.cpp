#include "RtpClientPush.h"
#include "Logger.h"
#include "RtpConnection.h"
#include "Common/Define.h"

using namespace std;

RtpClientPush::RtpClientPush(const string& ip, int port, const string& app, 
    const string& stream, int ssrc, int sockType)
    :RtpClient(ip, port, app, stream, ssrc, sockType, true)
    ,_peerIp(ip)
    ,_peerPort(port)
    ,_sockType(sockType)
    ,_ssrc(ssrc)
    ,_streamName(stream)
    ,_appName(app)
{
}

RtpClientPush::~RtpClientPush()
{
    logDebug << "~RtpClientPush()";
}


void RtpClientPush::onRead(const StreamBuffer::Ptr& buffer)
{
    
}

void RtpClientPush::doPush()
{
    if (!_connection) {
        if (_sockType == SOCKET_UDP) {
            _connection = make_shared<RtpConnectionSend>(getSocket()->getLoop(), getSocket(), 4);
        } else {
            _connection = make_shared<RtpConnectionSend>(getSocket()->getLoop(), getSocket(), 3);
        }
        _connection->setMediaInfo(_appName, _streamName, _ssrc);
        weak_ptr<RtpClientPush> wSelf = dynamic_pointer_cast<RtpClientPush>(shared_from_this());
        _connection->setOndetach([wSelf]() {
            logInfo << "to stop";
            auto self = wSelf.lock();
            if (self) {
                self->stop();
            }
        });
        _connection->onRead(nullptr, getAddr(), sizeof(sockaddr));
        _connection->init();
    }
}