#include "GB28181Client.h"
#include "Log/Logger.h"
#include "Common/Define.h"

using namespace std;

GB28181Client::GB28181Client(MediaClientType type, const string& app, 
    const string& stream, int ssrc, int sockType)
    :_type(type)
    ,_sockType(sockType)
    ,_ssrc(ssrc)
    ,_streamName(stream)
    ,_appName(app)
{
    _localUrlParser.path_ = "/" + _appName + "/" + _streamName;
    _localUrlParser.protocol_ = PROTOCOL_GB28181;
    _localUrlParser.type_ = DEFAULT_TYPE;
    _localUrlParser.vhost_ = DEFAULT_VHOST;
}

GB28181Client::~GB28181Client()
{
    
}

void GB28181Client::create(const string& localIp, int localPort, const string& url)
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
}

string GB28181Client::getLocalIp()
{
    if (!_socket) {
        return "";
    }
    return _socket->getLocalIp();
}

int GB28181Client::getLocalPort()
{
    if (!_socket) {
        return 0;
    }
    return _socket->getLocalPort();
}

EventLoop::Ptr GB28181Client::getLoop()
{
    if (!_socket) {
        return nullptr;
    }
    return _socket->getLoop();
}

bool GB28181Client::start(const string& localIp, int localPort, const string& url, int timeout)
{
    weak_ptr<GB28181Client> wSelf = shared_from_this();

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

    _socket->setErrorCb([wSelf](const std::string& err){
        logError << "gb28181 socket err: " << err;
        auto self = wSelf.lock();
        if (self) {
            self->stop();
        }
    });

    return true;
}

void GB28181Client::stop()
{
    logTrace << "GB28181Client::stop()";
    if (_onClose) {
        _onClose();
    }
}

void GB28181Client::pause()
{

}

void GB28181Client::setOnClose(const function<void()>& cb)
{
    _onClose = cb;
}

void GB28181Client::onConnected()
{
    _socket->setWriteCb(nullptr);
    if (_type == MediaClientType_Push) {
        doPush();
    } else {
        doPull();
    }
}

void GB28181Client::onRead(const StreamBuffer::Ptr& buffer)
{

}

void GB28181Client::doPush()
{
    
}

void GB28181Client::doPull()
{
    
}

void GB28181Client::send(const StreamBuffer::Ptr& buffer)
{
    _socket->send(buffer, 1);
}