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
    if (ip.empty()) {
        logWarn << "RtmpServer::start ip is empty";
        return ;
    }

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

void RtmpServer::stopListenByPort(int port, int count)
{
    lock_guard<mutex> lck(_mtx);
    auto iter = _tcpServers.find(port);
    if (iter == _tcpServers.end()) {
        return ;
    }
    for (auto& server : iter->second) {
        server->stop();
    }
    _delServers[port] = iter->second;
    _tcpServers.erase(port);

    auto loop = EventLoop::getCurrentLoop();
    weak_ptr<RtmpServer> wSelf = shared_from_this();
    loop->addTimerTask(2000, [wSelf, port](){
        auto self = wSelf.lock();
        if (!self) {
            return 0;
        }
        
        lock_guard<mutex> lck(self->_mtx);
        if (self->_delServers.find(port) == self->_delServers.end()) {
            return 0;
        }

        for (auto it = self->_delServers[port].begin(); it != self->_delServers[port].end();) {
            if ((*it)->getCurConnNum() == 0) {
                it =self->_delServers[port].erase(it);
            } else {
                ++it;
            }
        }

        if (self->_delServers[port].empty()) {
            self->_delServers.erase(port);
            return 0;
        }
        
        return 2000;
    }, nullptr);
}

void RtmpServer::for_each_server(const function<void(const TcpServer::Ptr &)> &cb)
{
    if (!cb) {
        return ;
    }
    
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _tcpServers) {
        for (auto& server : iter.second) {
            cb(server);
        }
    }
}