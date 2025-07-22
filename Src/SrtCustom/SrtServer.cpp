#include "SrtServer.h"
#include "Logger.h"
#include "EventLoopPool.h"
#include "SrtManager.h"

using namespace std;

SrtCustomServer::SrtCustomServer()
{}

SrtCustomServer::~SrtCustomServer()
{
    lock_guard<mutex> lck(_mtx);
    _udpSockets.clear();
}

SrtCustomServer::Ptr& SrtCustomServer::instance()
{
    static SrtCustomServer::Ptr instance = make_shared<SrtCustomServer>();
    return instance;
}

void SrtCustomServer::start(const string& ip, int port, int count, int sockType)
{
    SrtCustomServer::Wptr wSelf = shared_from_this();
    EventLoopPool::instance()->for_each_loop([ip, port, wSelf, sockType](const EventLoop::Ptr& loop){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        if (sockType == 2 || sockType == 3) {
            Socket::Ptr socket = make_shared<Socket>(loop);
            socket->createSocket(SOCKET_UDP);
            if (socket->bind(port, ip.data()) == -1) {
                logInfo << "bind udp failed, port: " << port;
                return ;
            }
            socket->addToEpoll();
            auto srtManager = SrtManager::instance();
            srtManager->init(loop);
            // socket->setOnGetRecvBuffer([](){
            //     auto buffer = make_shared<StreamBuffer>(1500 + 4);
            //     buffer->substr(4);

            //     return buffer;
            // });
            weak_ptr<Socket> weak_socket = socket; 
            socket->setReadCb([srtManager, weak_socket](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
                // create srtmanager
                auto strong_socket = weak_socket.lock();
                if (!strong_socket) {
                    return 0;
                }
                srtManager->onSrtPacket(strong_socket, buffer, addr, len);

                return 0;
            });
            socket->setRecvBuf(4 * 1024 * 1024);
            lock_guard<mutex> lck(self->_mtx);
            self->_udpSockets[port].emplace_back(socket);
        }
    });
}

void SrtCustomServer::stopByPort(int port, int count)
{
    lock_guard<mutex> lck(_mtx);
    _udpSockets.erase(port);
}

void SrtCustomServer::for_each_socket(const function<void(const Socket::Ptr &)> &cb)
{
    lock_guard<mutex> lck(_mtx);
    for (auto& iter : _udpSockets) {
        for (auto& socket : iter.second) {
            cb(socket);
        }
    }
}