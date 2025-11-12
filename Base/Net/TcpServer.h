#ifndef TcpServer_h
#define TcpServer_h

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "EventPoller/EventLoop.h"
#include "Socket.h"
#include "TcpConnection.h"

// using namespace std;



class TcpServer  : public std::enable_shared_from_this<TcpServer> {
public:
    using Ptr = std::shared_ptr<TcpServer>;
    using Wptr = std::weak_ptr<TcpServer>;
    using createSessionCb = std::function<TcpConnection::Ptr(const EventLoop::Ptr& loop, const Socket::Ptr& socket)>;
    TcpServer(EventLoop::Ptr loop, const std::string& host, int port, int maxConns, int threadNum);
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
    void initSession(int connFd);
    EventLoop::Ptr getLoop() {return _loop;}

    static TcpServer::Ptr getServerByLoop(int port, uint32_t index);
    static void addServer(int port, const TcpServer::Ptr& server);
    static void removeServer(int port, const EventLoop::Ptr& loop);

private:
    int _maxConns;
    int _threadNum;
    int _port;
    int _lastAcceptTime = 0;
    int _curConns = 0;
    uint32_t _serverIndex = 0;
    std::string _ip;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    std::unordered_map<int, TcpConnection::Ptr> _mapSession;
    createSessionCb _createSessionCb;

    static std::mutex _mutexServer;
    static std::unordered_map<int /*port*/, std::vector<TcpServer::Ptr>> _mapServer;
};

#endif //DnsCache_h