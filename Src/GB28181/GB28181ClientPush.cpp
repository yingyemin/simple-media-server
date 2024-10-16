#include "GB28181ClientPush.h"
#include "Logger.h"
#include "GB28181Connection.h"
#include "Common/Define.h"

using namespace std;

GB28181ClientPush::GB28181ClientPush(const string& ip, int port, const string& app, 
    const string& stream, int ssrc, int sockType)
    :GB28181Client(ip, port, app, stream, ssrc, sockType, true)
    ,_peerIp(ip)
    ,_peerPort(port)
    ,_sockType(sockType)
    ,_ssrc(ssrc)
    ,_streamName(stream)
    ,_appName(app)
{
}

GB28181ClientPush::~GB28181ClientPush()
{
    logDebug << "~GB28181ClientPush()";
}


void GB28181ClientPush::onRead(const StreamBuffer::Ptr& buffer)
{
    
}

void GB28181ClientPush::doPush()
{
    if (!_connection) {
        if (_sockType == SOCKET_UDP) {
            _connection = make_shared<GB28181ConnectionSend>(getSocket()->getLoop(), getSocket(), 4);
        } else {
            _connection = make_shared<GB28181ConnectionSend>(getSocket()->getLoop(), getSocket(), 3);
        }
        _connection->setMediaInfo(_appName, _streamName, _ssrc);
        weak_ptr<GB28181ClientPush> wSelf = dynamic_pointer_cast<GB28181ClientPush>(shared_from_this());
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