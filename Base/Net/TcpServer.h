#ifndef TcpServer_h
#define TcpServer_h

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "EventPoller/EventLoop.h"
#include "Socket.h"
#include "TcpConnection.h"

using namespace std;



class TcpServer  : public std::enable_shared_from_this<TcpServer> {
public:
    using Ptr = shared_ptr<TcpServer>;
    using Wptr = weak_ptr<TcpServer>;
    using createSessionCb = function<TcpConnection::Ptr(const EventLoop::Ptr& loop, const Socket::Ptr& socket)>;
    TcpServer(EventLoop::Ptr loop, const string& host, int port, int maxConns, int threadNum);
    ~TcpServer();

public:
    void start(NetType type = NET_IPV4);
    void stop();
    void accept(int event, void* args);
    TcpConnection::Ptr createSession(const EventLoop::Ptr& loop, const Socket::Ptr& socket);
    void setOnCreateSession(createSessionCb cb) {_createSessionCb = cb;}
    void onManager();
    int getPort() {return _port;}
    int getLastAcceptTime() {return _lastAcceptTime;}
    int getCurConnNum() {return _curConns;}

private:
    int _maxConns;
    int _threadNum;
    int _port;
    int _lastAcceptTime = 0;
    int _curConns = 0;
    string _ip;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    unordered_map<int, TcpConnection::Ptr> _mapSession;
    createSessionCb _createSessionCb;
};

#endif //DnsCache_h