#ifndef TcpConnection_h
#define TcpConnection_h

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "EventPoller/EventLoop.h"
#include "Socket.h"
#include "Buffer.h"
#include "Ssl/TlsContext.h"

using namespace std;



class TcpConnection  : public std::enable_shared_from_this<TcpConnection> {
public:
    using Ptr = shared_ptr<TcpConnection>;
    using Wptr = weak_ptr<TcpConnection>;
    using closeCb = function<void(TcpConnection::Ptr)>;
    TcpConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket);
    TcpConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket, bool enableTls);
    ~TcpConnection();

public:
    virtual void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len);
    virtual void onRecv(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len);
    virtual void onError();
    virtual void onManager();
    virtual void init() {}
    virtual void close();
    virtual ssize_t send(Buffer::Ptr pkt);

    // session结束时，从tcpserver中删除
    void setCloseCallback(closeCb cb) {_closeCb = cb;}
    Socket::Ptr getSocket() {return _socket;}
private:
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    TlsContext::Ptr _tlsCtx;
    closeCb _closeCb;
};

#endif //TcpConnection_h