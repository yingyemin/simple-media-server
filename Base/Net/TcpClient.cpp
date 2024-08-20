#include "TcpClient.h"
#include "Logger.h"

#include <cstring>
#include <iostream>

#include <errno.h>
#include <sys/epoll.h>
#include <netinet/in.h>

using namespace std;

TcpClient::TcpClient(const EventLoop::Ptr& loop)
{
    if (loop) {
        _loop = loop;
    } else {
        _loop = EventLoop::getCurrentLoop();
    }
}

TcpClient::TcpClient(const EventLoop::Ptr& loop, bool enableTls)
{
    _enableTls = enableTls;

    if (loop) {
        _loop = loop;
    } else {
        _loop = EventLoop::getCurrentLoop();
    }
}

TcpClient::~TcpClient()
{

}

int TcpClient::create(const string& localIp, int localPort)
{
    string bindIp = localIp;
    if (localIp.empty()) {
        bindIp = "0.0.0.0";
    }
    _socket = make_shared<Socket>(_loop);
    if (_socket->createSocket(SOCKET_TCP) < 0) {
        return -1;
    }
    if (_socket->bind(localPort, bindIp.data()) < 0) {
        return -1;
    }

    _socket->setReuseable();
    _socket->setNoSigpipe();
    _socket->setNoBlocked();
    _socket->setNoDelay();
    _socket->setSendBuf();
    _socket->setRecvBuf();
    _socket->setCloseWait();
    _socket->setCloExec();

    _socket->addToEpoll();

    weak_ptr<TcpClient> wSelf = shared_from_this();
    _socket->setReadCb([wSelf](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
        auto self = wSelf.lock();
        if (!self) {
            return -1;
        }
        self->onRecv(buffer, addr, len);
        return 0;
    });
    _socket->setErrorCb([wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return -1;
        }
        
        self->onError(strerror(errno));

        return 0;
    });
    _socket->setWriteCb([wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return -1;
        }
        
        self->onWrite();
        
        return 0;
    });

    _localIp = localIp;
    _localPort = _socket->getLocalPort();

    if (_enableTls) {
        _tlsCtx = make_shared<TlsContext>(false, _socket);

        _tlsCtx->setOnConnRead([this](const StreamBuffer::Ptr& buffer){
            onRead(buffer, nullptr, 0);
        });

        _tlsCtx->setOnConnSend([this](const Buffer::Ptr& buffer){
            send(buffer);
        });

        _tlsCtx->initSsl();
        _tlsCtx->handshake();
    }

    return 0;
}

int TcpClient::connect(const string& peerIp, int peerPort, int timeout)
{
    _peerIp = peerIp;
    _peerPort = peerPort;

    return _socket->connect(peerIp, peerPort, timeout);
}

void TcpClient::onRecv(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    if (_tlsCtx) {
        logInfo << "read a packet with ssl";
        _tlsCtx->onRead(buffer);
    } else {
        onRead(buffer, addr, len);
    }
}

void TcpClient::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{

}

void TcpClient::onWrite()
{
    if (_firstWrite) {
        onConnect();
        _firstWrite = false;
    }
}

void TcpClient::onError(const string& err)
{

}

void TcpClient::close() 
{
    if (_socket) {
        _socket->close();
        _socket = nullptr;
    }
    logInfo << "TcpClient::close";
}

ssize_t TcpClient::send(Buffer::Ptr pkt)
{
    if (!_socket) {
        return 0;
    }
    
    if (_tlsCtx) {
        return _tlsCtx->send(pkt);
    } else {
        return _socket->send(pkt);
    }
}