#include "TcpConnection.h"
#include "Log/Logger.h"

#include <cstring>
#include <iostream>

#include <errno.h>

using namespace std;

TcpConnection::TcpConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket)
    :_loop(loop)
    ,_socket(socket)
{

}

TcpConnection::TcpConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket, bool enableTls)
    :_loop(loop)
    ,_socket(socket)
{
    if (enableTls) {
        _tlsCtx = make_shared<TlsContext>(true, socket);

        _tlsCtx->setOnConnRead([this](const StreamBuffer::Ptr& buffer){
            onRead(buffer, nullptr, 0);
        });

        _tlsCtx->setOnConnSend([this](const Buffer::Ptr& buffer){
            send(buffer);
        });
        
        _tlsCtx->initSsl();
    }
}

TcpConnection::~TcpConnection()
{}

void TcpConnection::onRecv(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    if (_tlsCtx) {
        logInfo << "read a packet with ssl";
        _tlsCtx->onRead(buffer);
    } else {
        onRead(buffer, addr, len);
    }
}

void TcpConnection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    // _socket->onRead
    // struct sockaddr_storage addr;
    // socklen_t len = sizeof(addr);
    // while (true) {
    //     auto ret = _socket->read(_readBuffer, addr, len);
    //     if (ret < 0) {
    //         break;
    //     }
    //     // onRecv(_readBuffer, addr, len);
    // }
    logInfo << "read a packet";
}

void TcpConnection::onError()
{
    logError << "get a error";
}

void TcpConnection::onManager()
{
    logInfo << "manager";
}

void TcpConnection::close() 
{
    logInfo << "close";
    if (_closeCb) {
        auto self = shared_from_this();
        if (_loop->isCurrent()) {
            _closeCb(self);
        } else {
            _loop->async([self](){
                self->_closeCb(self);
            }, true, true);
        }
    }
}

ssize_t TcpConnection::send(Buffer::Ptr pkt)
{
    if (_tlsCtx) {
        return _tlsCtx->send(pkt);
    } else {
        return _socket->send(pkt);
    }
}
