#ifndef TcpClient_h
#define TcpClient_h

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "EventPoller/EventLoop.h"
#include "Ssl/TlsContext.h"
#include "Socket.h"

using namespace std;



class TcpClient  : public std::enable_shared_from_this<TcpClient> {
public:
    using Ptr = shared_ptr<TcpClient>;
    using Wptr = weak_ptr<TcpClient>;

    TcpClient(const EventLoop::Ptr& loop);
    TcpClient(const EventLoop::Ptr& loop, bool enableTls);
    ~TcpClient();

public:
    int create(const string& localIp, int localPort = 0);
    int connect(const string& peerIp, int peerPort, int timeout = 5);
    
    virtual void onRecv(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len);
    virtual void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len);
    virtual void onError(const string& err);
    virtual void onWrite();
    virtual void close();
    virtual void onConnect() {}
    ssize_t send(Buffer::Ptr pkt);

    int getLocalPort() {return _localPort;}
    int getPeerPort() {return _peerPort;}
    string getLocalIp() {return _localIp;}
    string getPeerIp() {return _peerIp;}
    Socket::Ptr getSocket() {return _socket;}

private:
    bool _firstWrite = true;
    bool _enableTls = false;
    int _localPort;
    int _peerPort;
    string _localIp;
    string _peerIp;
    TlsContext::Ptr _tlsCtx;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
};

#endif //TcpClient_h