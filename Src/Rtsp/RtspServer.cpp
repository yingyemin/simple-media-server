#include "RtspServer.h"
#include "Logger.h"
#include "EventLoopPool.h"
#include "RtspConnection.h"

using namespace std;

RtspServer::RtspServer()
{}

RtspServer::~RtspServer()
{
    logInfo << "~RtspServer()";
    lock_guard<mutex> lck(_mtx);
    _tcpServers.clear();
}

RtspServer::Ptr& RtspServer::instance()
{
    static RtspServer::Ptr instance = make_shared<RtspServer>();
    return instance;
}

void RtspServer::start(const string& ip, int port, int count, bool enableSsl)
{
    RtspServer::Wptr wSelf = shared_from_this();
    EventLoopPool::instance()->for_each_loop([ip, port, enableSsl, wSelf](const EventLoop::Ptr& loop){
        auto self = wSelf.lock();
        TcpServer::Ptr server = make_shared<TcpServer>(loop, ip.data(), port, 0, 0);
        server->setOnCreateSession([enableSsl](const EventLoop::Ptr& loop, const Socket::Ptr& socket) -> RtspConnection::Ptr {
            return make_shared<RtspConnection>(loop, socket, enableSsl);
        });
        server->start(Socket::getNetType(ip));
        lock_guard<mutex> lck(self->_mtx);
        self->_tcpServers[port].emplace_back(server);
    });
}

void RtspServer::stopByPort(int port, int count)
{
    lock_guard<mutex> lck(_mtx);
    _tcpServers.erase(port);
}

void RtspServer::for_each_server(const function<void(const TcpServer::Ptr &)> &cb)
{
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _tcpServers) {
        for (auto& server : iter.second) {
            cb(server);
        }
    }
}