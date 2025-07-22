#include "RtpServer.h"
#include "Logger.h"
#include "EventLoopPool.h"
#include "RtpConnection.h"
#include "RtpConnectionSend.h"
#include "RtpManager.h"

using namespace std;

RtpServer::RtpServer()
{}

RtpServer::~RtpServer()
{
    lock_guard<mutex> lck(_mtx);
    _tcpServers.clear();
}

RtpServer::Ptr& RtpServer::instance()
{
    static RtpServer::Ptr instance = make_shared<RtpServer>();
    return instance;
}

void RtpServer::startReceiveNoSsrc(const string& ip, int port, int sockType, const json& info)
{
    auto loop = EventLoop::getCurrentLoop();
    if (sockType == 1 || sockType == 3) {
        string streamName = info["streamName"];
        string appName = info["appName"];
        string uri = "/" + appName + "/" + streamName;
        TcpServer::Ptr server = make_shared<TcpServer>(loop, ip.data(), port, 0, 0);
        server->setOnCreateSession([uri](const EventLoop::Ptr& loop, const Socket::Ptr& socket) -> RtpConnection::Ptr {
            auto conn = make_shared<RtpConnection>(loop, socket);
            conn->setUri(uri);

            return conn;
        });
        server->start();
        lock_guard<mutex> lck(_mtx);
        _tcpServers[port].emplace_back(server);
    } 
    if (sockType == 2 || sockType == 3) {
        string streamName = info["streamName"];
        string appName = info["appName"];
        string uri = "/" + appName + "/" + streamName;

        Socket::Ptr socket = make_shared<Socket>(loop);
        socket->createSocket(SOCKET_UDP);
        if (socket->bind(port, ip.data()) == -1) {
            logInfo << "bind udp failed, port: " << port;
            return ;
        }
        socket->addToEpoll();
        // static auto gbManager = RtpManager::instance();
        // gbManager->init(loop);
        auto rtpContext = make_shared<RtpContext>(loop, uri, "vhost", "rtp", "normal");
        socket->setReadCb([rtpContext](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
            auto rtp = make_shared<RtpPacket>(buffer, 0);
            // create rtpmanager
            rtpContext->onRtpPacket(rtp, addr, len, true);

            return 0;
        });
        socket->setRecvBuf(4 * 1024 * 1024);
        lock_guard<mutex> lck(_mtx);
        _udpSockets[port].emplace_back(socket);
    }
}

void RtpServer::startReceive(const string& ip, int port, int sockType)
{
    auto loop = EventLoop::getCurrentLoop();
    if (sockType == 1 || sockType == 3) {
        TcpServer::Ptr server = make_shared<TcpServer>(loop, ip.data(), port, 0, 0);
        server->setOnCreateSession([](const EventLoop::Ptr& loop, const Socket::Ptr& socket) -> RtpConnection::Ptr {
            return make_shared<RtpConnection>(loop, socket);
        });
        server->start();
        lock_guard<mutex> lck(_mtx);
        _tcpServers[port].emplace_back(server);
    } 
    if (sockType == 2 || sockType == 3) {
        Socket::Ptr socket = make_shared<Socket>(loop);
        socket->createSocket(SOCKET_UDP);
        if (socket->bind(port, ip.data()) == -1) {
            logInfo << "bind udp failed, port: " << port;
            return ;
        }
        socket->addToEpoll();
        static auto gbManager = RtpManager::instance();
        gbManager->init(loop);
        socket->setReadCb([](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
            auto rtp = make_shared<RtpPacket>(buffer, 0);
            // create rtpmanager
            gbManager->onRtpPacket(rtp, addr, len);

            return 0;
        });
        socket->setRecvBuf(4 * 1024 * 1024);
        lock_guard<mutex> lck(_mtx);
        _udpSockets[port].emplace_back(socket);
    }
}

void RtpServer::startSend(const string& ip, int port, int sockType, 
                            const string& app, const string& stream, int ssrc)
{
    auto loop = EventLoop::getCurrentLoop();
    if (sockType == 1 || sockType == 3) {
        TcpServer::Ptr server = make_shared<TcpServer>(loop, ip.data(), port, 0, 0);
        server->setOnCreateSession([app, stream, ssrc](const EventLoop::Ptr& loop, const Socket::Ptr& socket) -> RtpConnectionSend::Ptr {
            auto connection = make_shared<RtpConnectionSend>(loop, socket, 1);
            connection->init();
            connection->setMediaInfo(app, stream, ssrc);

            return connection;
        });
        server->start();
        lock_guard<mutex> lck(_mtx);
        _tcpSendServers[port].emplace_back(server);
    } 
    if (sockType == 2 || sockType == 3) {
        Socket::Ptr socket = make_shared<Socket>(loop);
        socket->createSocket(SOCKET_UDP);
        if (socket->bind(port, ip.data()) == -1) {
            logInfo << "bind udp failed, port: " << port;
            return ;
        }
        socket->addToEpoll();
        auto connection = make_shared<RtpConnectionSend>(loop, socket, 2);
        connection->init();
        connection->setMediaInfo(app, stream, ssrc);
        socket->setReadCb([connection](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
            connection->onRead(buffer, addr, len);

            return 0;
        });
        socket->setRecvBuf(4 * 1024 * 1024);
        lock_guard<mutex> lck(_mtx);
        _udpSendSockets[port].emplace_back(socket);
    }
}

void RtpServer::start(const string& ip, int port, int count, int sockType)
{
    RtpServer::Wptr wSelf = shared_from_this();
    EventLoopPool::instance()->for_each_loop([ip, port, wSelf, sockType](const EventLoop::Ptr& loop){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        if (sockType == 1 || sockType == 3) {
            TcpServer::Ptr server = make_shared<TcpServer>(loop, ip.data(), port, 0, 0);
            server->setOnCreateSession([](const EventLoop::Ptr& loop, const Socket::Ptr& socket) -> RtpConnection::Ptr {
                return make_shared<RtpConnection>(loop, socket);
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
            auto rtpManager = RtpManager::instance();
            rtpManager->init(loop);
            // socket->setOnGetRecvBuffer([](){
            //     auto buffer = make_shared<StreamBuffer>(1500 + 4);
            //     buffer->substr(4);

            //     return buffer;
            // });
            socket->setReadCb([rtpManager](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
                // buffer->useAllBuffer();
                // auto rtp = make_shared<RtpPacket>(buffer, 0, 4);
                auto rtp = make_shared<RtpPacket>(buffer, 0);
                // create rtpmanager
                rtpManager->onRtpPacket(rtp, addr, len);

                return 0;
            });
            socket->setRecvBuf(4 * 1024 * 1024);
            lock_guard<mutex> lck(self->_mtx);
            self->_udpSockets[port].emplace_back(socket);
        }
    });
}

void RtpServer::stopByPort(int port, int count, int sockType)
{
    lock_guard<mutex> lck(_mtx);
    if (sockType == 1 || sockType == 3) {
        _tcpServers.erase(port);
    }
    if (sockType == 2 || sockType == 3) {
        _udpSockets.erase(port);
    }
}

void RtpServer::stopSendByPort(int port, int sockType)
{
    lock_guard<mutex> lck(_mtx);
    if (sockType == 1 || sockType == 3) {
        _tcpSendServers.erase(port);
    }
    if (sockType == 2 || sockType == 3) {
        _udpSendSockets.erase(port);
    }
}

void RtpServer::for_each_server(const function<void(const TcpServer::Ptr &)> &cb)
{
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _tcpServers) {
        for (auto& server : iter.second) {
            cb(server);
        }
    }
}

void RtpServer::for_each_socket(const function<void(const Socket::Ptr &)> &cb)
{
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _udpSockets) {
        for (auto& socket : iter.second) {
            cb(socket);
        }
    }
}