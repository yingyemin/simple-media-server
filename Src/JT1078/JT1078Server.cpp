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

void JT1078Server::setPortRange(int minPort, int maxPort)
{
    _portManager.init(minPort, maxPort);
}

void JT1078Server::setStreamPath(int port, const string& path, int expire, const string& appName)
{
    lock_guard<mutex> lck(_mtx);
    JT1078ServerInfo info;
    info.path_ = path;
    info.expire_ = expire;
    info.appName_ = appName;
    _serverInfo[port] = info;
}

void JT1078Server::start(const string& ip, int port, int count, bool isTalk)
{
    string path;
    string appName;
    int expire = 0;
    {
        lock_guard<mutex> lck(_mtx);
        auto info = _serverInfo.find(port);
        if (info != _serverInfo.end()) {
            path = _serverInfo[port].path_;
            appName = _serverInfo[port].appName_;
            expire = _serverInfo[port].expire_;
        }
    }
    
    JT1078Server::Wptr wSelf = shared_from_this();
    EventLoopPool::instance()->for_each_loop([ip, port, wSelf, path, expire, isTalk, count, appName](const EventLoop::Ptr& loop){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        TcpServer::Ptr server = make_shared<TcpServer>(loop, ip.data(), port, 0, 0);
        weak_ptr<TcpServer> wServer = server;
        server->setOnCreateSession([wSelf, wServer, path, expire, isTalk, count, appName](const EventLoop::Ptr& loop, const Socket::Ptr& socket) -> JT1078Connection::Ptr {
            auto self = wSelf.lock();
            if (!self) {
                return nullptr;
            }
            auto connection = make_shared<JT1078Connection>(loop, socket);
            if (!path.empty()) {
                connection->setPath(path);
            } else if (!appName.empty()) {
                connection->setAppName(appName);
            }
            if (isTalk) {
                connection->setTalkFlag();
            }

            if (expire) {
                int port = socket->getLocalPort();
                connection->setOnClose([wSelf, wServer, port, count](){
                    auto self = wSelf.lock();
                    auto server = wServer.lock();
                    // logTrace << "server->getCurConnNum(): " << server->getCurConnNum();
                    if (self && server && server->getCurConnNum() == 0) {
                        self->stopByPort(port, count);
                        // self->_portManager.put(port);
                    }
                });
            }
            return connection;
        });

        if (expire) {
            weak_ptr<TcpServer> weakServer = server;
            loop->addTimerTask(expire * 1000, [wSelf, weakServer, count](){
                auto self = wSelf.lock();
                if (!self) {
                    return 0;
                }

                auto server = weakServer.lock();
                if (!server) {
                    return 0;
                }

                if (server->getLastAcceptTime() == 0) {
                    self->stopByPort(server->getPort(), count);
                    // self->_portManager.put(server->getPort());
                }

                return 0;
            }, nullptr);
        }

        server->start();

        lock_guard<mutex> lck(self->_mtx);
        self->_tcpServers[port].emplace_back(server);
    }, count);
}

void JT1078Server::stopByPort(int port, int count)
{
    logTrace << "stop port: " << port;
    _portManager.put(port);

    lock_guard<mutex> lck(_mtx);
    if (_tcpServers.find(port) != _tcpServers.end()) {
        _tcpServers[port].clear();
        _tcpServers.erase(port);
    }
    
    _serverInfo.erase(port);
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