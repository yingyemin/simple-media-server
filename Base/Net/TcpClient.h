#ifndef TcpClient_h
#define TcpClient_h

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "EventPoller/EventLoop.h"
#include "Ssl/TlsContext.h"
#include "Socket.h"

//using namespace std;



class TcpClient  : public std::enable_shared_from_this<TcpClient> {
public:
    using Ptr = std::shared_ptr<TcpClient>;
    using Wptr = std::weak_ptr<TcpClient>;

    TcpClient(const EventLoop::Ptr& loop);
    TcpClient(const EventLoop::Ptr& loop, bool enableTls);
    ~TcpClient();

public:
    int create(const std::string& localIp, int localPort = 0);
    int connect(const std::string& peerIp, int peerPort, int timeout = 5);
    
    virtual void onRecv(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len);
    virtual void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len);
    virtual void onError(const std::string& err);
    virtual void onWrite();
    virtual void close();
    virtual void onConnect() {}
    ssize_t send(Buffer::Ptr pkt);

    int getLocalPort() {return _localPort;}
    int getPeerPort() {return _peerPort;}
    std::string getLocalIp() {return _localIp;}
    std::string getPeerIp() {return _peerIp;}
    Socket::Ptr getSocket() {return _socket;}
    EventLoop::Ptr getLoop() {return _loop;}

private:
    bool _firstWrite = true;
    bool _enableTls = false;
    int _localPort;
    int _peerPort;
    std::string _localIp;
    std::string _peerIp;
    TlsContext::Ptr _tlsCtx;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
};

#endif //TcpClient_h