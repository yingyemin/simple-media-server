#include "TcpServer.h"
#include "Logger.h"
#include "Util/TimeClock.h"
#include "EventPoller/EventLoopPool.h"

#include <cstring>
#include <iostream>

#include <errno.h>

#include "Util/Util.h"
//#include <sys/epoll.h>
//#include <netinet/in.h>

using namespace std;

mutex TcpServer::_mutexServer;
unordered_map<int /*port*/, std::vector<TcpServer::Ptr>> TcpServer::_mapServer;

TcpServer::TcpServer(EventLoop::Ptr loop, const string& host, int port, int maxConns, int threadNum)
    :_maxConns(maxConns)
    ,_threadNum(threadNum)
    ,_port(port)
    ,_ip(host)
    ,_loop(loop)
    ,_socket(make_shared<Socket>(_loop))
{
    logTrace << "TcpServer::TcpServer " << _ip << ":" << _port;
}

TcpServer::~TcpServer()
{
    logTrace << "TcpServer::~TcpServer " << _ip << ":" << _port;
}

void TcpServer::start(NetType type)
{
    addServer(_port, shared_from_this());

    _socket->createSocket(SOCKET_TCP, type == NET_IPV4 ? AF_INET : AF_INET6);
    _socket->bind(_port, _ip.data());
    _socket->listen(1024);

    TcpServer::Wptr weakServer = shared_from_this();
    _loop->addEvent(_socket->getFd(), EPOLLIN | EPOLLHUP | EPOLLERR | 0, [weakServer](int event, void* args){
        auto server = weakServer.lock();
        if (!server) {
            logInfo << "server exit";
            return ;
        }
        server->accept(event, args);
    }, nullptr);
    // std::bind(&TcpServer::accept, shared_from_this(), placeholders::_1, placeholders::_2), nullptr);

    _loop->addTimerTask(2000, [weakServer](){
        auto server = weakServer.lock();
        if (!server) {
            logInfo << "server exit";
            return 0;
        }
        server->onManager();
        return 2000;
    }, nullptr);
}

void TcpServer::stop()
{
    weak_ptr<TcpServer> wServer = shared_from_this();
    _loop->async([wServer](){
        auto server = wServer.lock();
        if (server && server->_socket) {
            server->_loop->delEvent(server->_socket->getFd(), nullptr);
            server->_socket->close();
            server->_socket = nullptr;
        }
    }, true);
}

void TcpServer::accept(int event, void* args)
{
    int connFd = -1;
    struct sockaddr_storage peer_addr;
    socklen_t addr_len = sizeof(peer_addr);

    _lastAcceptTime = TimeClock::now();

    weak_ptr<TcpServer> wServer = shared_from_this();
    while(true) {
        if (event & EPOLLIN) {
            do {
                connFd = (int)::accept(_socket->getFd(), (struct sockaddr*)&peer_addr, &addr_len);
            } while (0 /*-1 == fd && UV_EINTR == get_uv_error(true)*/);
        }

        if (connFd == -1) {
#if defined(_WIN32)
            int error = WSAGetLastError();
            if (error == WSAEINTR) {
                logWarn << "port: " << _port << ", accept error=WSAEINTR";
                continue;
                
            }
            // 建立链接过多，资源不够
            else if (error == WSAEMFILE){
                // 延时处理
                _loop->addTimerTask(100, [wServer, event, args]() {
                    auto server = wServer.lock();
                    if (server) {
                        server->accept(event, args);
                    }
                    return 0;
                    }, nullptr);
                logWarn << "port: " << _port << ", accept errno=EMFILE";
                break;
            }
            else if (error == WSAEWOULDBLOCK)
            {
                logDebug << "port: " << _port << ", accept errno=WSAEWOULDBLOCK";
                break;
            }
            else
            {
                logDebug << "port: " << _port << ", accept errno=EAGAIN";
                break;
            }
#else
            if (errno == EINTR) {
                logWarn << "port: " << _port << ", accept errno=EINTR";
                continue;
                //建立链接过多，资源不够
            }
            else if (errno == EMFILE) {
                // 延时处理
                _loop->addTimerTask(100, [wServer, event, args]() {
                    auto server = wServer.lock();
                    if (server) {
                        server->accept(event, args);
                    }
                    return 0;
                    }, nullptr);
                logWarn << "port: " << _port << ", accept errno=EMFILE";
                break;
                // 对方传输完毕, 为啥此处直接break?
            }
            else if (errno == EAGAIN) {
                logDebug << "port: " << _port << ", accept errno=EAGAIN";
                break;
            }
            else {
                logWarn << "port: " << _port << ", accept error";
                break;
            }
#endif
        }
        else {
#ifdef _WIN32
            auto anotherServer = getServerByLoop(_port, ++_serverIndex);
            if (!anotherServer) {
                logError << "port: " << _port << ", getServerByLoop error";
                continue;
            }
            auto loop = anotherServer->getLoop();
            loop->async([anotherServer, connFd](){
                anotherServer->initSession(connFd);
            }, true);
#else
            initSession(connFd);
#endif
            

            logTrace << "port: " << _port  << ",add session: " << _mapSession.size();
        }

        if (event & (EPOLLHUP | EPOLLERR)) {
            // ERROR
        }
    }
}

void TcpServer::initSession(int connFd)
{
    weak_ptr<TcpServer> wServer = shared_from_this();

    Socket::Ptr socket(new Socket(_loop, connFd));
    socket->setReuseable();
    socket->setNoSigpipe();
    socket->setNoBlocked();
    socket->setNoDelay();
    socket->setSendBuf();
    socket->setRecvBuf();
    socket->setCloseWait();
    socket->setCloExec();
    socket->setFamily(_socket->getFamily());
    socket->setZeroCopy();

    TcpConnection::Ptr session = createSession(_loop, socket);
    session->init();

    session->setCloseCallback([wServer](TcpConnection::Ptr session){
        auto server = wServer.lock();
        if (!server) {
            return ;
        }
        logTrace << "port: " << server->_port << ", close: " << session->getSocket()->getFd();
        server->_mapSession.erase(session->getSocket()->getFd());
        server->_curConns = server->_mapSession.size();
    });

    TcpConnection::Wptr weakSession = session;
    socket->setReadCb([weakSession](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
        auto session = weakSession.lock();
        if (!session) {
            return -1;
        }
        session->onRecv(buffer, addr, len);
        return 0;
    });
    socket->setErrorCb([weakSession](const std::string& errMsg){
        auto session = weakSession.lock();
        if (!session) {
            return -1;
        }
        session->onError(errMsg);
        return 0;
    });
    
    // logInfo << "add session";
    _mapSession.emplace(socket->getFd(), session);
    _curConns = _mapSession.size();            

    Socket::Wptr weakSocket = socket;
    _loop->addEvent(connFd, EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLOUT | 0, [weakSocket](int event, void* args){
        auto socket = weakSocket.lock();
        if (!socket) {
            return ;
        }
        socket->handleEvent(event, args);
    });
}

TcpConnection::Ptr TcpServer::createSession(const EventLoop::Ptr& loop, const Socket::Ptr& socket)
{
    if (_createSessionCb) {
        return _createSessionCb(loop, socket);
    } else {
        return make_shared<TcpConnection>(_loop, socket);
    }
}

void TcpServer::onManager()
{
    // logInfo << "manager: " << _mapSession.size();
    assert(_loop->isCurrent());
    for (auto &session : _mapSession) {
        //遍历时，可能触发onErr事件(也会操作_session_map)
        try {
            // onManger中不要执行close函数，会破坏map迭代器
            session.second->onManager();
        } catch (exception &ex) {
            logWarn << "port: " << _port << ", error: " << ex.what();
        }
    }
}

void TcpServer::addServer(int port, const TcpServer::Ptr& server)
{
    //logInfo << "add server: " << port << ", loop: " << server->getLoop()->getEpollFd();
    lock_guard<mutex> lock(_mutexServer);
    _mapServer[port].emplace_back(server);
}

TcpServer::Ptr TcpServer::getServerByLoop(int port, uint32_t index)
{
    //logInfo << "get server: " << port << ", index: " << index;
    lock_guard<mutex> lock(_mutexServer);
    if (_mapServer[port].empty()) {
        return nullptr;
    }
    index %= _mapServer[port].size();
    return _mapServer[port][index];
}

void TcpServer::removeServer(int port, const EventLoop::Ptr& loop)
{
    lock_guard<mutex> lock(_mutexServer);
    for (auto it = _mapServer[port].begin(); it != _mapServer[port].end();) {
        if ((*it)->getLoop() == loop) {
            it = _mapServer[port].erase(it);
        } else {
            ++it;
        }
    }
}