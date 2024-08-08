#include "WebrtcServer.h"
#include "WebrtcRtpPacket.h"
#include "Logger.h"
#include "EventLoopPool.h"
#include "WebrtcContextManager.h"
#include "WebrtcConnection.h"

using namespace std;

WebrtcServer::WebrtcServer()
{}

WebrtcServer::~WebrtcServer()
{
    lock_guard<mutex> lck(_mtx);
    _tcpServers.clear();
}

WebrtcServer::Ptr& WebrtcServer::instance()
{
    static WebrtcServer::Ptr instance = make_shared<WebrtcServer>();
    return instance;
}

void WebrtcServer::start(const string& ip, int port, int count, int sockType)
{
    WebrtcServer::Wptr wSelf = shared_from_this();
    EventLoopPool::instance()->for_each_loop([ip, port, wSelf, sockType](const EventLoop::Ptr& loop){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        if (sockType == 1 || sockType == 3) {
            TcpServer::Ptr server = make_shared<TcpServer>(loop, ip.data(), port, 0, 0);
            server->setOnCreateSession([](const EventLoop::Ptr& loop, const Socket::Ptr& socket) -> WebrtcConnection::Ptr {
                return make_shared<WebrtcConnection>(loop, socket);
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
            static auto rtpManager = WebrtcContextManager::instance();
            rtpManager->init(loop);
            socket->setReadCb([socket](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
                // create rtpmanager
                rtpManager->onUdpPacket(socket, buffer, addr, len);

                return 0;
            });
            socket->setRecvBuf(4 * 1024 * 1024);
            lock_guard<mutex> lck(self->_mtx);
            self->_udpSockets[port].emplace_back(socket);
        }
    });
}

void WebrtcServer::stopByPort(int port, int count, int sockType)
{
    lock_guard<mutex> lck(_mtx);
    if (sockType == 1 || sockType == 3) {
        _tcpServers.erase(port);
    }
    if (sockType == 2 || sockType == 3) {
        _udpSockets.erase(port);
    }
}

void WebrtcServer::for_each_server(const function<void(const TcpServer::Ptr &)> &cb)
{
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _tcpServers) {
        for (auto& server : iter.second) {
            cb(server);
        }
    }
}

void WebrtcServer::for_each_socket(const function<void(const Socket::Ptr &)> &cb)
{
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _udpSockets) {
        for (auto& socket : iter.second) {
            cb(socket);
        }
    }
}