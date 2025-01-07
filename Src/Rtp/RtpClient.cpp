#include "RtpClient.h"
#include "Logger.h"

using namespace std;

mutex RtpClient::_mtx;
unordered_map<string, RtpClient::Ptr> RtpClient::_mapClient;

RtpClient::RtpClient(const string& ip, int port, const string& app, 
    const string& stream, int ssrc, int sockType, bool sendFlag)
    :_sendFlag(sendFlag)
    ,_peerIp(ip)
    ,_peerPort(port)
    ,_sockType(sockType)
    ,_ssrc(ssrc)
    ,_streamName(stream)
    ,_appName(app)
{
}

RtpClient::~RtpClient()
{
    
}

void RtpClient::addClient(const string& key, const RtpClient::Ptr& client)
{
    lock_guard<mutex> lck(_mtx);
    if (_mapClient.find(key) != _mapClient.end()) {
        return ;
    }
    _mapClient[key] = client;
}


RtpClient::Ptr RtpClient::getClient(const string& key)
{
    lock_guard<mutex> lck(_mtx);
    if (_mapClient.find(key) == _mapClient.end()) {
        return nullptr;
    }
    return _mapClient[key];
}

void RtpClient::create()
{
    _socket = make_shared<Socket>(EventLoop::getCurrentLoop());
    if (_sockType == SOCKET_TCP) {
        _socket->createSocket(_sockType);
    } else {
        auto addr = _socket->createSocket(_peerIp, _peerPort, _sockType);
        _addr = *((sockaddr*)&addr);
    }
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

void RtpClient::start()
{
    weak_ptr<RtpClient> wSelf = shared_from_this();

    if (_sockType == SOCKET_TCP) {
        _socket->connect(_peerIp, _peerPort);
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

    _socket->setErrorCb([wSelf](){
        auto self = wSelf.lock();
        if (self) {
            self->stop();
        }
    });
}

void RtpClient::stop()
{
    logTrace << "RtpClient::stop()";
    string key = _peerIp + "_" + to_string(_peerPort) + "_" + to_string(_ssrc);
    lock_guard<mutex> lck(_mtx);
    _mapClient.erase(key);
}

void RtpClient::onConnected()
{
    _socket->setWriteCb(nullptr);
    if (_sendFlag) {
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