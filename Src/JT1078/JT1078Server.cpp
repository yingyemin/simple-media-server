#include "JT1078Server.h"
#include "Logger.h"
#include "EventLoopPool.h"
#include "JT1078Connection.h"

using namespace std;

JT1078Server::JT1078Server()
{}

JT1078Server::~JT1078Server()
{
    logInfo << "~JT1078Server()";
    lock_guard<mutex> lck(_mtx);
    _tcpServers.clear();
}

JT1078Server::Ptr& JT1078Server::instance(const string& key)
{
    static unordered_map<string, JT1078Server::Ptr> instance;
    
    auto iter = instance.find(key);
    if (iter == instance.end()) {
        auto server = make_shared<JT1078Server>();
        server->setServerId(key);
        instance[key] = server;
        return instance[key];
    }

    return iter->second;
}

JT1078Server::Ptr& JT1078Server::instance()
{
    static auto instance = make_shared<JT1078Server>();
    return instance;
}

void JT1078Server::setServerId(const string& key)
{
    _serverId = key;
}

void JT1078Server::start(const string& ip, int port, int count)
{
    JT1078Server::Wptr wSelf = shared_from_this();
    EventLoopPool::instance()->for_each_loop([ip, port, wSelf](const EventLoop::Ptr& loop){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        TcpServer::Ptr server = make_shared<TcpServer>(loop, ip.data(), port, 0, 0);
        server->setOnCreateSession([wSelf](const EventLoop::Ptr& loop, const Socket::Ptr& socket) -> JT1078Connection::Ptr {
            auto self = wSelf.lock();
            if (!self) {
                return nullptr;
            }
            auto connection = make_shared<JT1078Connection>(loop, socket);
            return connection;
        });

        server->start();

        lock_guard<mutex> lck(self->_mtx);
        self->_tcpServers[port].emplace_back(server);
    });
}

void JT1078Server::stopByPort(int port, int count)
{
    lock_guard<mutex> lck(_mtx);
    _tcpServers.erase(port);
}

void JT1078Server::for_each_server(const function<void(const TcpServer::Ptr &)> &cb)
{
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _tcpServers) {
        for (auto& server : iter.second) {
            cb(server);
        }
    }
}