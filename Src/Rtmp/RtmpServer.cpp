#include "RtmpServer.h"
#include "Logger.h"
#include "EventLoopPool.h"
#include "RtmpConnection.h"

using namespace std;

RtmpServer::RtmpServer()
{}

RtmpServer::~RtmpServer()
{
    lock_guard<mutex> lck(_mtx);
    _tcpServers.clear();
}

RtmpServer::Ptr& RtmpServer::instance()
{
    static RtmpServer::Ptr instance = make_shared<RtmpServer>();
    return instance;
}

void RtmpServer::start(const string& ip, int port, int count)
{
    RtmpServer::Wptr wSelf = shared_from_this();
    EventLoopPool::instance()->for_each_loop([ip, port, wSelf](const EventLoop::Ptr& loop){
        auto self = wSelf.lock();
        TcpServer::Ptr server = make_shared<TcpServer>(loop, ip.data(), port, 0, 0);
        server->setOnCreateSession([](const EventLoop::Ptr& loop, const Socket::Ptr& socket) -> RtmpConnection::Ptr {
            return make_shared<RtmpConnection>(loop, socket);
        });
        server->start();
        lock_guard<mutex> lck(self->_mtx);
        self->_tcpServers[port].emplace_back(server);
    });
}

void RtmpServer::stopByPort(int port, int count)
{
    lock_guard<mutex> lck(_mtx);
    _tcpServers.erase(port);
}

void RtmpServer::for_each_server(const function<void(const TcpServer::Ptr &)> &cb)
{
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _tcpServers) {
        for (auto& server : iter.second) {
            cb(server);
        }
    }
}