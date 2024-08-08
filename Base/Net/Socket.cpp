#include "Socket.h"
#include "DnsCache.h"
#include "Logger.h"

#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <sys/epoll.h>

using namespace std;



static int setIpv6Only(int fd, bool flag)
{
    int opt = flag;
    int ret = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&opt, sizeof opt);
    if (ret == -1) {
        cout << "setsockopt IPV6_V6ONLY failed";
    }
    return ret;
}

static int bindSockIpv6(int fd, const char *ifr_ip, uint16_t port) {
    setIpv6Only(fd, false);
    struct sockaddr_in6 addr;
    bzero(&addr, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    if (1 != inet_pton(AF_INET6, ifr_ip, &(addr.sin6_addr))) {
        if (strcmp(ifr_ip, "0.0.0.0")) {
            cout << "inet_pton to ipv6 address failed: " << ifr_ip;
        }
        addr.sin6_addr = IN6ADDR_ANY_INIT;
    }
    if (::bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        cout << "Bind socket failed: ";
        return -1;
    }
    return 0;
}

static int bindSockIpv4(int fd, const char *ifr_ip, uint16_t port) {
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (1 != inet_pton(AF_INET, ifr_ip, &(addr.sin_addr))) {
        if (strcmp(ifr_ip, "::")) {
            logWarn << "inet_pton to ipv4 address failed: " << ifr_ip;
        }
        addr.sin_addr.s_addr = INADDR_ANY;
    }
    if (::bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        logError << "Bind socket failed: ";
        return -1;
    }
    return 0;
}

static int bindSock(int fd, const char *ifr_ip, uint16_t port, int family) {
    switch (family) {
        case AF_INET: return bindSockIpv4(fd, ifr_ip, port);
        case AF_INET6: return bindSockIpv6(fd, ifr_ip, port);
        default: return -1;
    }
}

Socket::Socket(const EventLoop::Ptr& loop)
    :_loop(loop)
    ,_sendBuffer(make_shared<SocketBuffer>())
{
    logInfo << "Socket(" << this << ")";
}

Socket::Socket(const EventLoop::Ptr& loop, int fd)
    :_loop(loop)
    ,_fd(fd)
    ,_sendBuffer(make_shared<SocketBuffer>())
{
    logInfo << "Socket(" << this << ")";
}

Socket::~Socket()
{
    logInfo << "~Socket(" << this << "), fd: " << _fd;
    close();
}

sockaddr_storage Socket::createSocket(const string& peerIp, int peerPort, int type)
{
    sockaddr_storage addr;
    //优先使用ipv4地址
    if (!DnsCache::instance().getDomainIP(peerIp.data(), peerPort, addr, AF_INET, SOCK_STREAM, IPPROTO_TCP)) {
        //dns解析失败
        logError << "get domain ip failed";
        return addr;
    }

    _family = addr.ss_family;
    switch (type)
    {
    case 1:
        createTcpSocket(addr.ss_family);
        break;
    
    case 2:
        createUdpSocket(addr.ss_family);
        break;

    default:
        cerr << "unsurpport socket protocol";
        break;
    }

    return addr;
}



int Socket::createSocket(int type)
{
    // int family = support_ipv6() ? (is_ipv4(local_ip) ? AF_INET : AF_INET6) : AF_INET;
    _family = AF_INET;
    switch (type)
    {
    case 1:
        return createTcpSocket(_family);
        break;
    
    case 2:
        return createUdpSocket(_family);
        break;

    default:
        cerr << "unsurpport socket protocol";
        break;
    }

    return -1;
}

int Socket::createTcpSocket(int family)
{
    _type = SOCKET_TCP;
    _fd = (int) socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (_fd < 0) {
        logError << "Create tcp socket failed: ";
        return -1;
    }

    setReuseable();
    setNoSigpipe();
    setNoBlocked();
    setNoDelay();
    setSendBuf(256 * 1024);
    setRecvBuf();
    setCloseWait();
    setCloExec();

    return 0;
}

int Socket::createUdpSocket(int family)
{
    _type = SOCKET_UDP;
    _fd = (int) socket(family, SOCK_DGRAM, IPPROTO_UDP);
    if (_fd < 0) {
        logError << "Create udp socket failed: ";
        return -1;
    }

    setReuseable();
    setNoSigpipe();
    setNoBlocked();
    // setNoDelay();
    setSendBuf();
    setRecvBuf();
    setCloseWait();
    setCloExec();

    logTrace << "create a udp socket: " << _fd;

    return 0;
}

int Socket::setReuseable()
{
    int opt = 1;
    int ret = setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret == -1) {
        cout << "setsockopt SO_REUSEADDR failed";
        return ret;
    }
    
    ret = setsockopt(_fd, SOL_SOCKET, SO_REUSEPORT, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret == -1) {
        cout << "setsockopt SO_REUSEPORT failed";
    }
    
    return ret;
}

int Socket::setNoSigpipe()
{
#if defined(SO_NOSIGPIPE)
    int set = 1;
    auto ret = setsockopt(_fd, SOL_SOCKET, SO_NOSIGPIPE, (char *) &set, sizeof(int));
    if (ret == -1) {
        cout << "setsockopt SO_NOSIGPIPE failed";
    }
    return ret;
#else
    return -1;
#endif
}

int Socket::setNoBlocked()
{
    int ul = 1;
    int ret = ioctl(_fd, FIONBIO, &ul); //设置为非阻塞模式
    if (ret == -1) {
        cout << "ioctl FIONBIO failed";
    }

    return ret;
}

int Socket::setNoDelay()
{
    int opt = 1;
    int ret = setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret == -1) {
        cout << "setsockopt TCP_NODELAY failed";
    }
    return ret;
}

int Socket::setSendBuf(int size)
{
    if (size <= 0) {
        return 0;
    }
    int ret = setsockopt(_fd, SOL_SOCKET, SO_SNDBUF, (char *) &size, sizeof(size));
    if (ret == -1) {
        cout << "setsockopt SO_SNDBUF failed";
    }
    return ret;
}

int Socket::setRecvBuf(int size)
{
    if (size <= 0) {
        // use the system default value
        return 0;
    }
    int ret = setsockopt(_fd, SOL_SOCKET, SO_RCVBUF, (char *) &size, sizeof(size));
    if (ret == -1) {
        cout << "setsockopt SO_RCVBUF failed";
    }
    return ret;
}

int Socket::setCloseWait(int second)
{
    linger m_sLinger;
    //在调用closesocket()时还有数据未发送完，允许等待
    // 若m_sLinger.l_onoff=0;则调用closesocket()后强制关闭
    m_sLinger.l_onoff = (second > 0);
    m_sLinger.l_linger = second; //设置等待时间为x秒
    int ret = setsockopt(_fd, SOL_SOCKET, SO_LINGER, (char *) &m_sLinger, sizeof(linger));
    if (ret == -1) {
        cout << "setsockopt SO_LINGER failed";
    }
    return ret;
}

int Socket::setCloExec()
{
    int on = 1;
    int flags = fcntl(_fd, F_GETFD);
    if (flags == -1) {
        cout << "fcntl F_GETFD failed";
        return -1;
    }
    if (on) {
        flags |= FD_CLOEXEC;
    } else {
        int cloexec = FD_CLOEXEC;
        flags &= ~cloexec;
    }
    int ret = fcntl(_fd, F_SETFD, flags);
    if (ret == -1) {
        cout << "fcntl F_SETFD failed";
        return -1;
    }
    return ret;
}

int Socket::setKeepAlive(int interval, int idle, int times)
{
    // Enable/disable the keep-alive option
    int on = 1;
    int opt = 1;
    int ret = setsockopt(_fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret == -1) {
        cout << "setsockopt SO_KEEPALIVE failed";
    }
#if !defined(_WIN32)
#if !defined(SOL_TCP) && defined(IPPROTO_TCP)
  #define SOL_TCP IPPROTO_TCP
#endif
#if !defined(TCP_KEEPIDLE) && defined(TCP_KEEPALIVE)
  #define TCP_KEEPIDLE TCP_KEEPALIVE
#endif
    // Set the keep-alive parameters
    if (on && interval > 0 && ret != -1) {
        ret = setsockopt(_fd, SOL_TCP, TCP_KEEPIDLE, (char *) &idle, static_cast<socklen_t>(sizeof(idle)));
        if (ret == -1) {
            cout << "setsockopt TCP_KEEPIDLE failed";
        }
        ret = setsockopt(_fd, SOL_TCP, TCP_KEEPINTVL, (char *) &interval, static_cast<socklen_t>(sizeof(interval)));
        if (ret == -1) {
            cout << "setsockopt TCP_KEEPINTVL failed";
        }
        ret = setsockopt(_fd, SOL_TCP, TCP_KEEPCNT, (char *) &times, static_cast<socklen_t>(sizeof(times)));
        if (ret == -1) {
            cout << "setsockopt TCP_KEEPCNT failed";
        }
    }
#endif
    return ret;
}

bool Socket::bindPeerAddr(const struct sockaddr *dst_addr)
{
    if (_type != SOCKET_UDP) {
        return false;
    }

    socklen_t addr_len = sizeof(struct sockaddr);
    return 0 == ::connect(_fd, dst_addr, addr_len);
}

int Socket::bind(const uint16_t port, const char *localIp)
{
    // int family = support_ipv6() ? (is_ipv4(local_ip) ? AF_INET : AF_INET6) : AF_INET;
    // int family = AF_INET;
    if (bindSock(_fd, localIp, port, _family) == -1) {
        close();
        logInfo << "bind socket failed, ip: " << localIp << ", port: " << port;
        return -1;
    }

    return 0;
}

int Socket::listen(int backlog)
{
    //开始监听
    if (::listen(_fd, backlog) == -1) {
        cout << "Listen socket failed: ";
        close();
        return -1;
    }

    return 0;
}

int Socket::connect(const string& peerIp, int port, int timeout)
{
    _isClient = true;
    sockaddr_storage addr;
    //优先使用ipv4地址
    if (!DnsCache::instance().getDomainIP(peerIp.data(), port, addr, AF_INET, SOCK_STREAM, IPPROTO_TCP)) {
        //dns解析失败
        return -1;
    }

    struct timeval timeo = {3, 0};
    timeo.tv_sec = timeout;
    setsockopt(_fd, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));

    ((sockaddr_in *) &addr)->sin_port = htons(port);
    errno = 0;
    if (::connect(_fd, (sockaddr *) &addr, sizeof(sockaddr_in)) == 0) {
        addToEpoll();
        //同步连接成功
        return 0;
    }
    
    logInfo << "errno: " << errno;
    if (errno == 115) {
        addToEpoll();
        weak_ptr<Socket> wSelf = shared_from_this();
        _loop->addTimerTask(timeout * 1000, [wSelf](){
            logInfo << "time task start";
            auto self = wSelf.lock();
            if (!self) {
                return 0;
            }
            logInfo << "time task connect: " << self->_isConnected;
            if (self->_isConnected) {
                return 0;
            }
            self->onError(nullptr);
            return 0;
        }, [](bool success, const shared_ptr<TimerTask>& task){});

        return 0;
    }

    return -1;
}

void Socket::handleEvent(int event, void* args)
{
    // logInfo << "handle event: " << event;
    // logInfo << "EPOLLIN: " << EPOLLIN;
    // logInfo << "EPOLLOUT: " << EPOLLOUT;
    // logInfo << "EPOLLERR: " << EPOLLERR;
    // logInfo << "EPOLLHUP: " << EPOLLHUP;
    if (event & EPOLLIN) {
        // logInfo << "handle read: " << this;
        onRead(args);
    } 
    if (event & EPOLLOUT) {
        logInfo << "handle write: " << this;
        if (!_isConnected) {
            _isConnected = true;
        }
        onWrite(args);
    } 
    if (event & EPOLLERR || event & EPOLLHUP) {
        logInfo << "handle error";
        onError(args);
    }
}

void Socket::addToEpoll()
{
    Socket::Wptr weakSocket = shared_from_this();
    _loop->addEvent(_fd, EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLOUT | EPOLLET, [weakSocket](int event, void* args){
        auto socket = weakSocket.lock();
        if (!socket) {
            return ;
        }
        socket->handleEvent(event, args);
    });
}

thread_local StreamBuffer::Ptr g_readBuffer;
thread_local StreamBuffer::Ptr g_writeBuffer;
int Socket::onRead(void* args)
{
    if (!g_readBuffer) {
        g_readBuffer = StreamBuffer::create();
        g_readBuffer->setCapacity(1 + 4 * 1024 * 1024);
    }
    ssize_t ret = 0, nread = 0;
    auto data = g_readBuffer->data();
    // 最后一个字节设置为'\0'
    auto capacity = g_readBuffer->getCapacity() - 1;

    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    while (true) {
        do {
            nread = recvfrom(_fd, data, capacity, 0, (struct sockaddr *)&addr, &len);
        } while (-1 == nread && EINTR == errno);

        if (nread == 0) {
            if (_type == 1) {
                onError(nullptr);
            } else {
                logInfo << "Recv eof on udp socket[" << _fd << "]";
            }
            return ret;
        }

        if (nread == -1) {
            if (errno != EAGAIN) {
                if (_type == 1) {
                    cout << errno;
                } else {
                    cout << "Recv err on udp socket[" << _fd << "]: " << strerror(errno);
                }
            }
            return ret;
        }

        ret += nread;
        data[nread] = '\0';
        // 设置buffer有效数据大小
        g_readBuffer->setSize(nread);

        // 触发回调
        // LOCK_GUARD(_mtx_event);
        try {
            // 此处捕获异常，目的是防止数据未读尽，epoll边沿触发失效的问题
            _onRead(g_readBuffer, (struct sockaddr *)&addr, len);
        } catch (std::exception &ex) {
            cout << "Exception occurred when emit on_read: " << ex.what();
        }
    }
    return 0;
}

int Socket::onWrite(void* args)
{
    // if (!g_writeBuffer) {
    //     g_writeBuffer = StreamBuffer::create();
    //     g_writeBuffer->setCapacity(1 + 4 * 1024 * 1024);
    // }
    if (_onWrite) {
        _onWrite();
    }

    if (_readyBuffer.size() == 0 && _sendBuffer->length == 0) {
        logInfo << "no buffer to send";
        _loop->modifyEvent(_fd, EPOLLIN | EPOLLHUP | EPOLLERR | 0, nullptr);
        return 0;
    }

    send(nullptr);

    return 0;
}

int Socket::onError(void* args)
{
    logError << "get a socket err: " << strerror(errno);
    weak_ptr<Socket> weakSelf = shared_from_this();
    _loop->async([weakSelf](){
        auto self = weakSelf.lock();
        if (!self) {
            return ;
        }
        // self->close();
        self->_onError();
    }, true, false);

    return 0;
}

bool Socket::onGetBuffer()
{
    // logTrace << "Socket::onGetBuffer()";
    if (_onGetBuffer) {
        // logTrace << "_onGetBuffer";
        return _onGetBuffer();
    }

    return false;
}

int Socket::close()
{
    if (_fd > 0) {
        _loop->delEvent(_fd, [](bool success){});
        ::close(_fd);
        _fd = 0;
    }

    return 0;
}

ssize_t Socket::send(const char* data, int len, int flag, struct sockaddr *addr, socklen_t addr_len)
{
    auto buffer = make_shared<StreamBuffer>(data, len);
    return send(buffer, flag, 0, addr, addr_len);
}

ssize_t Socket::send(const Buffer::Ptr pkt, int flag, int offset, struct sockaddr *addr, socklen_t addr_len)
{
    
    if (!_loop->isCurrent()) { //切换到当前socket线程发送
        Socket::Wptr wSelf = shared_from_this();
        _loop->async([wSelf, pkt, flag, offset, addr, addr_len](){
            auto self = wSelf.lock();
            if (self) {
                self->send(pkt, flag, offset, addr, addr_len);
            }
        }, true, true);
        logInfo << "change thread";
        return 0;
    }
    // 需要改为配置读取
    // 超过缓存了，丢掉pkt
    if (_remainSize > 10 * 1024 * 1024) {
        if (flag && _sendBuffer->length > 0) {
            // _readyBuffer.push_back(_sendBuffer);
            // _sendBuffer = make_shared<SocketBuffer>();
        } else {
            // pkt = nullptr;
            // flag = false;
        }
        // logInfo << "overlow buffer: " << _remainSize;
    }

    if (pkt) {
        // logInfo << "send pkt size: " << pkt->size() << ", flag : " << flag;
        iovec io;
        int size = pkt->size() - offset;
        io.iov_base = pkt->data() + offset;
        io.iov_len = size;

        _sendBuffer->vecBuffer.emplace_back(std::move(io));
        _sendBuffer->rawBuffer.push_back(pkt);
        _sendBuffer->length += size;
        _sendBuffer->addr = addr;
        _sendBuffer->addr_len = addr_len;

        _remainSize += size;
    } else {
        // logInfo << "send pkt size: " << 0 << ", flag : " << flag;
    }

    if (flag || _sendBuffer->vecBuffer.size() > 1000) {
        _readyBuffer.push_back(_sendBuffer);
        _sendBuffer = make_shared<SocketBuffer>();
    }

    int readySize = _readyBuffer.size();
    if (readySize == 0) {
        // logInfo << "_readyBuffer empty";
        return 0;
    }

    // logInfo << "_remainSize: " << _remainSize;

    ssize_t totalSendSize = 0;
    for (int i = 0; i < readySize; ++i) {
        auto& sendBuffer = _readyBuffer.front();
        if (sendBuffer->length == 0) {
            logInfo << "sendBuffer->length is 0";
            _readyBuffer.pop_front();
            continue;
        }

        ssize_t sendSize = 0;
        do {
            struct msghdr msg = {0};
            msg.msg_iov = &(sendBuffer->vecBuffer[0]);
            msg.msg_iovlen = sendBuffer->vecBuffer.size();
            msg.msg_name = sendBuffer->addr;
            msg.msg_namelen = sendBuffer->addr_len;
            
            sendSize = sendmsg(_fd, &msg, MSG_DONTWAIT | MSG_NOSIGNAL);
        } while (sendSize == -1 && errno == EINTR);

        // logInfo << "sendBuffer->length: " << sendBuffer->length;
        // logInfo << "sendSize: " << sendSize;

        if (sendSize >= sendBuffer->length) {
            // logInfo << "sendBuffer->length: " << sendBuffer->length;
            totalSendSize += sendBuffer->length;
            _readyBuffer.pop_front();
            continue;
        } else if (sendSize > 0) {
            totalSendSize += sendSize;
            for (auto it = sendBuffer->vecBuffer.begin(); it != sendBuffer->vecBuffer.end();) {
                size_t size = it->iov_len;
                if (sendSize >= size) {
                    sendSize -= size;
                    it = sendBuffer->vecBuffer.erase(it);
                    // sendBuffer->rawBuffer.pop_front();
                    sendBuffer->length -= size;
                } else {
                    it->iov_base = (char*)(it->iov_base) + sendSize;
                    it->iov_len -= sendSize;
                    sendBuffer->length -= sendSize;
                    break;
                }
            }
            logInfo << "sendBuffer->length: " << sendBuffer->length;
            break;
        } else {
            break;
        }
    }

    // if (pkt) {
    //     iovec io;
    //     io.iov_base = pkt->data();
    //     io.iov_len = pkt->size();
    //     _remainSize += io.iov_len;

    //     vio.emplace_back(io);
    // }  

    // do {
    //     struct msghdr msg = {0};
    //     msg.msg_iov = &(vio[0]);
    //     msg.msg_iovlen = _sendBuffer.size() + 1;
    //     sendSize = sendmsg(_fd, &msg, MSG_DONTWAIT | MSG_NOSIGNAL);
    // } while (sendSize == -1 && errno == EINTR);

    // if (sendSize >= _remainSize) { //全部发送完了
    //     _sendBuffer.clear();
    //     return sendSize;
    // } else if (sendSize > 0) {
    //     if (pkt) {
    //         _sendBuffer.emplace_back(pkt);
    //     }
    //     for (auto it = _sendBuffer.begin(); it != _sendBuffer.end();) {
    //         size_t size = it->get()->size();
    //         if (sendSize > size) {
    //             sendSize -= size;
    //             it = _sendBuffer.erase(it);
    //         } else {
    //             it->get()->setup(sendSize);
    //             break;
    //         }
    //     }
    // } else {
    //     if (pkt) {
    //         _sendBuffer.emplace_back(pkt);
    //     }
    // }

    _remainSize -= totalSendSize;
    // logInfo << "_remainSize: " << _remainSize;
    // logInfo << "totalSendSize: " << totalSendSize;

    if (_remainSize > 0) {
        _loop->modifyEvent(_fd, EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR | 0, nullptr);
    } else {
        // 上层根据实际情况，是发后面的buffer，还是断开链接
        // 可能会造成递归问题
        onGetBuffer();
    }

    return totalSendSize;
}

int Socket::getLocalPort() {
    if (_localPort != -1) {
        return _localPort;
    }

    struct sockaddr_in localaddr;
    socklen_t len = sizeof(localaddr);
    int ret = getsockname(_fd, (struct sockaddr*)&localaddr, &len);
    if (ret != 0) {
        logWarn << "get sock name failed";
        return -1;
    } else {
        string ip;
        ip.resize(INET_ADDRSTRLEN);
        _localPort = ntohs(localaddr.sin_port);
        _localIp = inet_ntop(AF_INET, &(localaddr.sin_addr), (char*)ip.data(), INET_ADDRSTRLEN);
        return _localPort;
    }
}

string Socket::getLocalIp() {
    if (!_localIp.empty()) {
        return _localIp;
    }

    struct sockaddr_in localaddr;
    socklen_t len = sizeof(localaddr);
    int ret = getsockname(_fd, (struct sockaddr*)&localaddr, &len);
    if (ret != 0) {
        logWarn << "get sock name failed";
        return "";
    } else {
        string ip;
        ip.resize(INET_ADDRSTRLEN);
        _localPort = ntohs(localaddr.sin_port);
        _localIp = inet_ntop(AF_INET, &(localaddr.sin_addr), (char*)ip.data(), INET_ADDRSTRLEN);
        return _localIp;
    }
}

int Socket::getPeerPort() {
    if (_peerPort != -1) {
        return _peerPort;
    }

    struct sockaddr_in peerAddr;
    socklen_t len = sizeof(peerAddr);
    int ret = getpeername(_fd, (struct sockaddr*)&peerAddr, &len);
    if (ret != 0) {
        logWarn << "get sock name failed";
        return -1;
    } else {
        string ip;
        ip.resize(INET_ADDRSTRLEN);
        _peerPort = ntohs(peerAddr.sin_port);
        _peerIp = inet_ntop(AF_INET, &(peerAddr.sin_addr), (char*)ip.data(), INET_ADDRSTRLEN);
        return _peerPort;
    }
}

string Socket::getPeerIp() {
    if (!_peerIp.empty()) {
        return _peerIp;
    }

    struct sockaddr_in peerAddr;
    socklen_t len = sizeof(peerAddr);
    int ret = getpeername(_fd, (struct sockaddr*)&peerAddr, &len);
    if (ret != 0) {
        logWarn << "get sock name failed";
        return "";
    } else {
        string ip;
        ip.resize(INET_ADDRSTRLEN);
        _peerPort = ntohs(peerAddr.sin_port);
        _peerIp = inet_ntop(AF_INET, &(peerAddr.sin_addr), (char*)ip.data(), INET_ADDRSTRLEN);
        return _peerIp;
    }
}

sockaddr* Socket::getPeerAddr()
{
    socklen_t len = sizeof(_peerAddr);
    int ret = getpeername(_fd, (struct sockaddr*)&_peerAddr, &len);
    if (ret != 0) {
        logWarn << "get sock name failed";
        return nullptr;
    }

    return (struct sockaddr*)&_peerAddr;
}

int Socket::getSocketType()
{
    int protocol;
    socklen_t protocolLength = sizeof(protocol);
 
    // 获取socket的传输协议
    if (getsockopt(_fd, SOL_SOCKET, SO_PROTOCOL, &protocol, &protocolLength) == -1) {
        logError << "Error getting socket protocol" << std::endl;
        return -1;
    }

    return protocol == IPPROTO_TCP ? SOCKET_TCP : SOCKET_UDP;
}
