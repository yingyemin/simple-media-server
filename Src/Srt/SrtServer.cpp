#include "SrtServer.h"
#include "Logger.h"
#include "EventLoopPool.h"

using namespace std;

#ifdef ENABLE_SRT

SrtServer::SrtServer()
{}

SrtServer::~SrtServer()
{
}

SrtServer::Ptr& SrtServer::instance()
{
    static SrtServer::Ptr instance = make_shared<SrtServer>();
    return instance;
}

void SrtServer::start(const string& ip, int port, int count, int sockType)
{
    static int listened = false;
    SrtServer::Wptr wSelf = shared_from_this();
    SrtEventLoopPool::instance()->for_each_loop([ip, port, wSelf, sockType](const SrtEventLoop::Ptr& loop){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        SrtSocket::Ptr socket = make_shared<SrtSocket>(loop, true);
        socket->createSocket(0);
        if (socket->bind(port, ip.data()) == -1) {
            logInfo << "bind udp failed, port: " << port;
            return ;
        }
        
        // if (!listened) {
            logInfo << "start listen =====================";
            if (socket->listen(1024) == -1) {
                logInfo << "listen udp failed, port: " << port;
                return ;
            }
            // listened = true;
        // }

        logInfo << "socket fd: " << socket->getFd();
        socket->addToEpoll();
        socket->setAcceptCb([socket, wSelf](){
            auto acceptFd = socket->accept();
            auto acceptSocket = make_shared<SrtSocket>(socket->getLoop(), acceptFd);
            acceptSocket->addToEpoll();

            auto conn = make_shared<SrtConnection>(socket->getLoop(), acceptSocket);
            
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }
            // int acceptFd = acceptSocket->getFd();
            logTrace << "add srt conn: " << acceptFd;
            self->_mapSrtConn.emplace(acceptFd, conn);

            conn->setOnClose([wSelf, acceptFd](){
                auto self = wSelf.lock();
                if (!self) {
                    return ;
                }
                logTrace << "erase srt conn: " << acceptFd;
                self->_mapSrtConn.erase(acceptFd);
            });
            conn->init();
            weak_ptr<SrtConnection> wConn = conn;
            acceptSocket->setReadCb([wConn, wSelf](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
                auto conn = wConn.lock();
                if (!conn) {
                    return 0;
                }
                conn->onRead(buffer, addr, len);
                return 0;
            });
        });
        // static auto rtpManager = WebrtcContextManager::instance();
        // rtpManager->init(loop);
        // socket->setReadCb([socket, wSelf](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
        //     // create rtpmanager
        //     // rtpManager->onUdpPacket(socket, buffer, addr, len);

        //     auto acceptFd = socket->accept();
        //     auto acceptSocket = make_shared<SrtSocket>(socket->getLoop(), acceptFd);

        //     auto conn = make_shared<SrtConnection>(socket->getLoop(), socket);
        //     conn->onRead(buffer, addr, len);

        //     auto self = wSelf.lock();
        //     if (!self) {
        //         return -1;
        //     }
        //     self->_mapSrtConn.emplace(socket->getFd(), conn);

        //     return 0;
        // });
        // // socket->setRecvBuf(4 * 1024 * 1024);
        lock_guard<mutex> lck(self->_mtx);
        self->_udpSockets[port].emplace_back(socket);
    });
}

void SrtServer::stopByPort(int port, int count, int sockType)
{
    lock_guard<mutex> lck(_mtx);
    _udpSockets.erase(port);
}

void SrtServer::for_each_socket(const function<void(const SrtSocket::Ptr &)> &cb)
{
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _udpSockets) {
        for (auto& socket : iter.second) {
            cb(socket);
        }
    }
}

#endif