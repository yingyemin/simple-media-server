#include "Ehome5Server.h"
#include "Logger.h"
#include "EventLoopPool.h"
#include "Ehome5Connection.h"
// #include "GB28181/GB28181Manager.h"

using namespace std;

Ehome5Server::Ehome5Server()
{}

Ehome5Server::~Ehome5Server()
{
    lock_guard<mutex> lck(_mtx);
    _tcpServers.clear();
}

Ehome5Server::Ptr& Ehome5Server::instance()
{
    static Ehome5Server::Ptr instance = make_shared<Ehome5Server>();
    return instance;
}

void Ehome5Server::start(const string& ip, int port, int count, int sockType)
{
    Ehome5Server::Wptr wSelf = shared_from_this();
    EventLoopPool::instance()->for_each_loop([ip, port, wSelf, sockType](const EventLoop::Ptr& loop){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        if (sockType == 1 || sockType == 3) {
            TcpServer::Ptr server = make_shared<TcpServer>(loop, ip.data(), port, 0, 0);
            server->setOnCreateSession([](const EventLoop::Ptr& loop, const Socket::Ptr& socket) -> Ehome5Connection::Ptr {
                return make_shared<Ehome5Connection>(loop, socket);
            });
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
        //     static auto gbManager = GB28181Manager::instance();
        //     gbManager->init(loop);
        //     socket->setReadCb([](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
        //         auto rtp = make_shared<RtpPacket>(buffer, 0);
        //         // create rtpmanager
        //         gbManager->onRtpPacket(rtp, addr, len);

        //         return 0;
        //     });
        //     socket->setRecvBuf(4 * 1024 * 1024);
        //     lock_guard<mutex> lck(self->_mtx);
        //     self->_udpSockets[port].emplace_back(socket);
        // }
    });
}

void Ehome5Server::stopByPort(int port, int count, int sockType)
{
    lock_guard<mutex> lck(_mtx);
    if (sockType == 1 || sockType == 3) {
        _tcpServers.erase(port);
    }
    if (sockType == 2 || sockType == 3) {
        _udpSockets.erase(port);
    }
}

void Ehome5Server::for_each_server(const function<void(const TcpServer::Ptr &)> &cb)
{
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _tcpServers) {
        for (auto& server : iter.second) {
            cb(server);
        }
    }
}

void Ehome5Server::for_each_socket(const function<void(const Socket::Ptr &)> &cb)
{
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _udpSockets) {
        for (auto& socket : iter.second) {
            cb(socket);
        }
    }
}