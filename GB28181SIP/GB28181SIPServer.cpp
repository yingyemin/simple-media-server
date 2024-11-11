#include "GB28181SIPServer.h"
#include "GB28181SIPConnection.h"
#include "GB28181SIPManager.h"
#include "Logger.h"
#include "EventLoopPool.h"

using namespace std;

GB28181SIPServer::GB28181SIPServer()
{}

GB28181SIPServer::~GB28181SIPServer()
{
    lock_guard<mutex> lck(_mtx);
    _tcpServers.clear();
}

GB28181SIPServer::Ptr& GB28181SIPServer::instance()
{
    static GB28181SIPServer::Ptr instance = make_shared<GB28181SIPServer>();
    return instance;
}

void GB28181SIPServer::start(const string& ip, int port, int count, int sockType)
{
    GB28181SIPServer::Wptr wSelf = shared_from_this();
    EventLoopPool::instance()->for_each_loop([ip, port, wSelf, sockType](const EventLoop::Ptr& loop){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        if (sockType == 1 || sockType == 3) {
            TcpServer::Ptr server = make_shared<TcpServer>(loop, ip.data(), port, 0, 0);
            server->setOnCreateSession([](const EventLoop::Ptr& loop, const Socket::Ptr& socket) -> GB28181SIPConnection::Ptr {
                return make_shared<GB28181SIPConnection>(loop, socket);
            });
            server->start();
            lock_guard<mutex> lck(self->_mtx);
            self->_tcpServers[port].emplace_back(server);
        } 
        if (sockType == 2 || sockType == 3) {
            Socket::Ptr socket = make_shared<Socket>(loop);
            socket->createSocket(SOCKET_UDP);
            if (socket->bind(port, ip.data()) == -1) {
                logInfo << "bind udp failed, port: " << port;
                return ;
            }
            socket->addToEpoll();
            static auto gbManager = GB28181SIPManager::instance();
            gbManager->init(loop);
            socket->setReadCb([socket](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
                // auto rtp = make_shared<RtpPacket>(buffer, 0);
                // create rtpmanager
                gbManager->onSipPacket(socket, buffer, addr, len);

                return 0;
            });
            socket->setRecvBuf(4 * 1024 * 1024);
            lock_guard<mutex> lck(self->_mtx);
            self->_udpSockets[port].emplace_back(socket);
        }
    });
}

void GB28181SIPServer::stopByPort(int port, int count, int sockType)
{
    lock_guard<mutex> lck(_mtx);
    if (sockType == 1 || sockType == 3) {
        _tcpServers.erase(port);
    }
    if (sockType == 2 || sockType == 3) {
        _udpSockets.erase(port);
    }
}

void GB28181SIPServer::for_each_server(const function<void(const TcpServer::Ptr &)> &cb)
{
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _tcpServers) {
        for (auto& server : iter.second) {
            cb(server);
        }
    }
}

void GB28181SIPServer::for_each_socket(const function<void(const Socket::Ptr &)> &cb)
{
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _udpSockets) {
        for (auto& socket : iter.second) {
            cb(socket);
        }
    }
}