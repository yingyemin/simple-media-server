#include "HttpServer.h"
#include "Logger.h"
#include "EventLoopPool.h"
#include "HttpConnection.h"
#include "WebsocketConnection.h"

using namespace std;

HttpServer::HttpServer()
{}

HttpServer::~HttpServer()
{
    logInfo << "~HttpServer()";
    lock_guard<mutex> lck(_mtx);
    _tcpServers.clear();
}

HttpServer::Ptr& HttpServer::instance(const string& key)
{
    static unordered_map<string, HttpServer::Ptr> instance;
    
    auto iter = instance.find(key);
    if (iter == instance.end()) {
        auto server = make_shared<HttpServer>();
        server->setServerId(key);
        instance[key] = server;
        return instance[key];
    }

    return iter->second;
}

HttpServer::Ptr& HttpServer::instance()
{
    static auto instance = make_shared<HttpServer>();
    return instance;
}

void HttpServer::setServerId(const string& key)
{
    _serverId = key;
}

void HttpServer::start(const string& ip, int port, int count, bool enableSsl, bool isWebsocket)
{
    HttpServer::Wptr wSelf = shared_from_this();
    EventLoopPool::instance()->for_each_loop([ip, port, wSelf, enableSsl, isWebsocket](const EventLoop::Ptr& loop){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        TcpServer::Ptr server = make_shared<TcpServer>(loop, ip.data(), port, 0, 0);
        server->setOnCreateSession([wSelf, enableSsl, isWebsocket](const EventLoop::Ptr& loop, const Socket::Ptr& socket) -> HttpConnection::Ptr {
            auto self = wSelf.lock();
            if (!self) {
                return nullptr;
            }
            if (isWebsocket) {
                return make_shared<WebsocketConnection>(loop, socket, enableSsl);
            }
            auto connection = make_shared<HttpConnection>(loop, socket, enableSsl);
            connection->setServerId(self->_serverId);
            return connection;
        });

        server->start();

        lock_guard<mutex> lck(self->_mtx);
        self->_tcpServers[port].emplace_back(server);
    }, count);
}

void HttpServer::stopByPort(int port, int count)
{
    lock_guard<mutex> lck(_mtx);
    _tcpServers.erase(port);
}

void HttpServer::for_each_server(const function<void(const TcpServer::Ptr &)> &cb)
{
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _tcpServers) {
        for (auto& server : iter.second) {
            cb(server);
        }
    }
}