#include "RtpClient.h"
#include "Logger.h"
#include "Common/Define.h"

using namespace std;


RtpClient::RtpClient(MediaClientType type, const string& app, 
    const string& stream, int ssrc, int sockType)
    :_type(type)
    ,_sockType(sockType)
    ,_ssrc(ssrc)
    ,_streamName(stream)
    ,_appName(app)
{
    _localUrlParser.path_ = "/" + _appName + "/" + _streamName;
    _localUrlParser.protocol_ = PROTOCOL_RTP;
    _localUrlParser.type_ = DEFAULT_TYPE;
    _localUrlParser.vhost_ = DEFAULT_VHOST;
}

RtpClient::~RtpClient()
{
    
}

void RtpClient::create(const string& localIp, int localPort, const string& url)
{
    _peerUrlParser.parse(url);
    logDebug << "_peerUrlParser.host_: " << _peerUrlParser.host_ << ", _peerUrlParser.port_: " << _peerUrlParser.port_;
    
    if (_peerUrlParser.port_ == 0 || _peerUrlParser.host_.empty()) {
        logInfo << "peer ip: " << _peerUrlParser.host_ << ", peerPort: " 
                << _peerUrlParser.port_ << ", failed: is empty";
        return ;
    }

    _socket = make_shared<Socket>(EventLoop::getCurrentLoop());
    if (_sockType == SOCKET_TCP) {
        _socket->createSocket(_sockType);
    } else {
        auto addr = _socket->createSocket(_peerUrlParser.host_, _peerUrlParser.port_, _sockType);
        _addr = *((sockaddr*)&addr);
    }
    _socket->bind(0, "0.0.0.0");
}

string RtpClient::getLocalIp()
{
    if (!_socket) {
        return "";
    }
    return _socket->getLocalIp();
}

int RtpClient::getLocalPort()
{
    if (!_socket) {
        return 0;
    }
    return _socket->getLocalPort();
}

EventLoop::Ptr RtpClient::getLoop()
{
    if (!_socket) {
        return nullptr;
    }
    return _socket->getLoop();
}

bool RtpClient::start(const string& localIp, int localPort, const string& url, int timeout)
{
    weak_ptr<RtpClient> wSelf = shared_from_this();

    if (_sockType == SOCKET_TCP) {
        _socket->connect(_peerUrlParser.host_, _peerUrlParser.port_, timeout);
        _socket->setWriteCb([wSelf](){
            auto self = wSelf.lock();
            if (self) {
                self->onConnected();
            }
        });
    } else {
        _socket->bindPeerAddr(&_addr);
        onConnected();
    }

    _socket->setReadCb([wSelf](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
        auto self = wSelf.lock();
        if (self) {
            self->onRead(buffer);
        }

        return 0;
    });

    _socket->setErrorCb([wSelf](const std::string& errMsg){
        auto self = wSelf.lock();
        if (self) {
            self->stop();
        }

        logDebug << "RtpClient::onError " << errMsg;
    });

    return true;
}

void RtpClient::stop()
{
    logTrace << "RtpClient::stop()";
    if (_onClose) {
        _onClose();
    }
}

void RtpClient::pause()
{

}

void RtpClient::setOnClose(const function<void()>& cb)
{
    _onClose = cb;
}

void RtpClient::onConnected()
{
    _socket->setWriteCb(nullptr);
    if (_type == MediaClientType_Push) {
        doPush();
    } else {
        doPull();
    }
}

void RtpClient::onRead(const StreamBuffer::Ptr& buffer)
{

}

void RtpClient::doPush()
{
    
}

void RtpClient::doPull()
{
    
}

void RtpClient::send(const StreamBuffer::Ptr& buffer)
{
    _socket->send(buffer, 1);
}