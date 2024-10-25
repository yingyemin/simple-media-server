#include "GB28181Client.h"
#include "Logger.h"

using namespace std;

mutex GB28181Client::_mtx;
unordered_map<string, GB28181Client::Ptr> GB28181Client::_mapClient;

GB28181Client::GB28181Client(const string& ip, int port, const string& app, 
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

GB28181Client::~GB28181Client()
{
    
}

void GB28181Client::addClient(const string& key, const GB28181Client::Ptr& client)
{
    lock_guard<mutex> lck(_mtx);
    if (_mapClient.find(key) != _mapClient.end()) {
        return ;
    }
    _mapClient[key] = client;
}


GB28181Client::Ptr GB28181Client::getClient(const string& key)
{
    lock_guard<mutex> lck(_mtx);
    if (_mapClient.find(key) == _mapClient.end()) {
        return nullptr;
    }
    return _mapClient[key];
}

void GB28181Client::create()
{
    _socket = make_shared<Socket>(EventLoop::getCurrentLoop());
    if (_sockType == SOCKET_TCP) {
        _socket->createSocket(_sockType);
    } else {
        auto addr = _socket->createSocket(_peerIp, _peerPort, _sockType);
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

void GB28181Client::start()
{
    weak_ptr<GB28181Client> wSelf = shared_from_this();

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

void GB28181Client::stop()
{
    logTrace << "GB28181Client::stop()";
    string key = _peerIp + "_" + to_string(_peerPort) + "_" + to_string(_ssrc);
    lock_guard<mutex> lck(_mtx);
    _mapClient.erase(key);
}

void GB28181Client::onConnected()
{
    _socket->setWriteCb(nullptr);
    if (_sendFlag) {
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