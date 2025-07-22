#include "JT808Server.h"
#include "Logger.h"
#include "EventLoopPool.h"
#include "JT808Connection.h"

using namespace std;

JT808Server::JT808Server()
{}

JT808Server::~JT808Server()
{
    logInfo << "~JT808Server()";
    lock_guard<mutex> lck(_mtx);
    _tcpServers.clear();
}

JT808Server::Ptr& JT808Server::instance(const string& key)
{
    static unordered_map<string, JT808Server::Ptr> instance;
    
    auto iter = instance.find(key);
    if (iter == instance.end()) {
        auto server = make_shared<JT808Server>();
        server->setServerId(key);
        instance[key] = server;
        return instance[key];
    }

    return iter->second;
}

JT808Server::Ptr& JT808Server::instance()
{
    static auto instance = make_shared<JT808Server>();
    return instance;
}

void JT808Server::setServerId(const string& key)
{
    _serverId = key;
}

void JT808Server::start(const string& ip, int port, int count)
{
    JT808Server::Wptr wSelf = shared_from_this();
    EventLoopPool::instance()->for_each_loop([ip, port, wSelf, count](const EventLoop::Ptr& loop){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        TcpServer::Ptr server = make_shared<TcpServer>(loop, ip.data(), port, 0, 0);
        server->setOnCreateSession([wSelf](const EventLoop::Ptr& loop, const Socket::Ptr& socket) -> JT808Connection::Ptr {
            auto self = wSelf.lock();
            if (!self) {
                return nullptr;
            }
            
            return make_shared<JT808Connection>(loop, socket);
        });

        server->start();

        lock_guard<mutex> lck(self->_mtx);
        self->_tcpServers[port].emplace_back(server);
    }, count);
}

void JT808Server::stopByPort(int port, int count)
{
    lock_guard<mutex> lck(_mtx);
    if (_tcpServers.find(port) != _tcpServers.end()) {
        _tcpServers[port].clear();
        _tcpServers.erase(port);
    }
}

void JT808Server::for_each_server(const function<void(const TcpServer::Ptr &)> &cb)
{
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _tcpServers) {
        for (auto& server : iter.second) {
            cb(server);
        }
    }
}