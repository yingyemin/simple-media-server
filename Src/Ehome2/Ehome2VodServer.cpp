#include "Ehome2VodServer.h"
#include "Logger.h"
#include "EventLoopPool.h"
#include "Ehome2VodConnection.h"
#include "Rtp/RtpManager.h"

using namespace std;

Ehome2VodServer::Ehome2VodServer()
{}

Ehome2VodServer::~Ehome2VodServer()
{
    lock_guard<mutex> lck(_mtx);
    _tcpServers.clear();
}

Ehome2VodServer::Ptr& Ehome2VodServer::instance()
{
    static Ehome2VodServer::Ptr instance = make_shared<Ehome2VodServer>();
    return instance;
}

void Ehome2VodServer::setServerInfo(int port, const string& path, int expire, const string& appName, int timeout)
{
    lock_guard<mutex> lck(_mtx);
    Ehome2VodServerInfo info;
    info.path_ = path;
    info.expire_ = expire;
    info.appName_ = appName;
    info.timeout_ = timeout;
    _serverInfo[port] = info;
}

void Ehome2VodServer::start(const string& ip, const string& streamId, int port, int sockType)
{
    int expire = 0;
    uint64_t timeout = 0;
    {
        lock_guard<mutex> lck(_mtx);
        auto info = _serverInfo.find(port);
        if (info != _serverInfo.end()) {
            expire = _serverInfo[port].expire_;
            timeout = _serverInfo[port].timeout_;
        }
    }

    Ehome2VodServer::Wptr wSelf = shared_from_this();
    EventLoopPool::instance()->for_each_loop([ip, port, wSelf, sockType, streamId, expire, timeout](const EventLoop::Ptr& loop){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        if (sockType == 1 || sockType == 3) {
            TcpServer::Ptr server = make_shared<TcpServer>(loop, ip.data(), port, 0, 0);
            weak_ptr<TcpServer> wServer = server;
            server->setOnCreateSession([wSelf, wServer, streamId, expire, timeout](const EventLoop::Ptr& loop, const Socket::Ptr& socket) -> Ehome2VodConnection::Ptr {
                auto connection = make_shared<Ehome2VodConnection>(loop, socket, streamId, timeout);
                if (expire) {
                    int port = socket->getLocalPort();
                    connection->setOnClose([wSelf, wServer, port](){
                        auto self = wSelf.lock();
                        auto server = wServer.lock();
                        // logTrace << "server->getCurConnNum(): " << server->getCurConnNum();
                        if (self && server && server->getCurConnNum() == 0) {
                            self->stopByPort(port, 3);
                            // self->_portManager.put(port);
                        }
                    });
                }

                return connection;
            });

            if (expire) {
                loop->addTimerTask(expire, [wSelf, wServer](){
                    auto self = wSelf.lock();
                    if (!self) {
                        return 0;
                    }

                    auto server = wServer.lock();
                    if (!server) {
                        return 0;
                    }

                    if (server->getLastAcceptTime() == 0) {
                        self->delServer(server);
                        // self->_portManager.put(server->getPort());
                    }

                    return 0;
                }, nullptr);
            }

            server->start();
            lock_guard<mutex> lck(self->_mtx);
            self->_tcpServers[port].emplace_back(server);
        } 
        // if (sockType == 2 || sockType == 3) {
        //     Socket::Ptr socket = make_shared<Socket>(loop);
        //     socket->createSocket(SOCKET_UDP);
        //     if (socket->bind(port, ip.data()) == -1) {
        //         logInfo << "bind udp failed, port: " << port;
        //         return ;
        //     }
        //     socket->addToEpoll();
        //     static auto rtpManager = RtpManager::instance();
        //     rtpManager->init(loop);
        //     socket->setReadCb([](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
        //         auto rtp = make_shared<RtpPacket>(buffer, 0);
        //         // create rtpmanager
        //         rtpManager->onRtpPacket(rtp, addr, len);

        //         return 0;
        //     });
        //     socket->setRecvBuf(4 * 1024 * 1024);
        //     lock_guard<mutex> lck(self->_mtx);
        //     self->_udpSockets[port].emplace_back(socket);
        // }
    });
}

void Ehome2VodServer::stopByPort(int port, int sockType)
{
    lock_guard<mutex> lck(_mtx);
    if (sockType == 1 || sockType == 3) {
        _tcpServers.erase(port);
    }
    if (sockType == 2 || sockType == 3) {
        _udpSockets.erase(port);
    }
}

void Ehome2VodServer::delServer(const TcpServer::Ptr& server)
{
    lock_guard<mutex> lck(_mtx);
    auto iter = _tcpServers.find(server->getPort());
    if (iter == _tcpServers.end()) {
        return;
    }
    
    for (auto serverIt = iter->second.begin(); serverIt != iter->second.end(); ++serverIt) {
        if (*serverIt == server) {
            iter->second.erase(serverIt);
            break;
        }
    }

    if (iter->second.empty()) {
        _tcpServers.erase(iter);
    }
}

void Ehome2VodServer::for_each_server(const function<void(const TcpServer::Ptr &)> &cb)
{
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _tcpServers) {
        for (auto& server : iter.second) {
            cb(server);
        }
    }
}

void Ehome2VodServer::for_each_socket(const function<void(const Socket::Ptr &)> &cb)
{
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _udpSockets) {
        for (auto& socket : iter.second) {
            cb(socket);
        }
    }
}