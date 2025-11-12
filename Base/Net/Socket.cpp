#include "Socket.h"
#include "DnsCache.h"
#include "Logger.h"

#include <cstring>
#include <unistd.h>
#include <algorithm>

using namespace std;

static int setIpv6Only(int fd, bool flag)
{
    int opt = flag;
    int ret = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&opt, sizeof opt);
    if (ret == -1) {
        logWarn << "setsockopt IPV6_V6ONLY failed";
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
            logWarn << "inet_pton to ipv6 address failed: " << ifr_ip;
        }
        addr.sin6_addr = IN6ADDR_ANY_INIT;
    }
    if (::bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        logWarn << "Bind socket failed: " << strerror(errno);
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
        logError << "Bind socket port(" << port << ") failed: " << strerror(errno);
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
    logTrace << "Socket(" << this << ")";
    _createTime = time(NULL);
}

Socket::Socket(const EventLoop::Ptr& loop, int fd)
    :_loop(loop)
    ,_fd(fd)
    ,_sendBuffer(make_shared<SocketBuffer>())
{
    logTrace << "Socket(" << this << ")";
    _createTime = time(NULL);
}

Socket::~Socket()
{
    logTrace << "~Socket(" << this << "), fd: " << _fd;
    close();
}

sockaddr_storage Socket::createSocket(const string& peerIp, int peerPort, int type)
{
    sockaddr_storage addr;
    int netType = getNetType(peerIp);
    int ss_family = AF_INET;
    if (netType == NET_IPV6) {
        ss_family = AF_INET6;
    }
    
    int ipproto = IPPROTO_TCP;
    if (type == SOCKET_UDP) {
        ipproto = IPPROTO_UDP;
    }

    //优先使用ipv4地址
    if (!DnsCache::instance().getDomainIP(peerIp.data(), peerPort, addr, ss_family, SOCK_STREAM, ipproto)) {
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
        logWarn << "unsurpport socket protocol";
        break;
    }

    return addr;
}



int Socket::createSocket(int type, int family)
{
    // int family = support_ipv6() ? (is_ipv4(local_ip) ? AF_INET : AF_INET6) : AF_INET;
    _family = family;
    switch (type)
    {
    case 1:
        return createTcpSocket(_family);
        break;
    
    case 2:
        return createUdpSocket(_family);
        break;

    default:
        logWarn << "unsurpport socket protocol";
        break;
    }

    return -1;
}

int Socket::createTcpSocket(int family)
{
    _type = SOCKET_TCP;
    _fd = (int)socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (_fd < 0) {
        logError << "Create tcp socket failed: " << strerror(errno);
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
    setIpv6Only(false);
    setZeroCopy();

    return 0;
}

int Socket::createUdpSocket(int family)
{
    _type = SOCKET_UDP;
    _fd = (int)socket(family, SOCK_DGRAM, IPPROTO_UDP);
    if (_fd < 0) {
        logError << "Create udp socket failed: " << strerror(errno);
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
    setIpv6Only(false);
    setZeroCopy();

    logDebug << "###create a udp socket: " << _fd;

    return 0;
}

int Socket::setReuseable()
{
    int opt = 1;
    int ret = setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret == -1) {
        logWarn << "setsockopt SO_REUSEADDR failed: " << strerror(errno);
        return ret;
    }
#ifndef _WIN32
    ret = setsockopt(_fd, SOL_SOCKET, SO_REUSEPORT, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret == -1) {
        logWarn << "setsockopt SO_REUSEPORT failed: " << strerror(errno);
    }
#endif
    return ret;
}

int Socket::setIpv6Only(bool enable)
{
    if (_family != AF_INET6) {
        return 0;
    }

    int ret = setsockopt(_fd, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<char*>(&enable), sizeof(enable));
    if (ret == -1)
    {
#ifndef _WIN32
        logWarn << "setsockopt IPV6_V6ONLY failed: " << strerror(errno);
#endif
    }
    
    return ret;
}

int Socket::setZeroCopy()
{
#if defined(MSG_ZEROCOPY) &&	\
    defined(SO_ZEROCOPY) &&	\
    defined(SOL_SOCKET) && 0
    int enable;
    int ret = setsockopt(_fd, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable));
    if (ret == -1)
    {
        logWarn << "setsockopt SO_ZEROCOPY failed: " << strerror(errno);
    }
    
    return ret;
#else
    logDebug << "socket not support zero copy";
    return 0;
#endif
}

int Socket::setNoSigpipe()
{
#if defined(SO_NOSIGPIPE)
    int set = 1;
    auto ret = setsockopt(_fd, SOL_SOCKET, SO_NOSIGPIPE, (char *) &set, sizeof(int));
    if (ret == -1) {
        logWarn << "setsockopt SO_NOSIGPIPE failed: " << strerror(errno);
    }
    return ret;
#else
    return -1;
#endif
}

int Socket::setNoBlocked()
{
#if defined(_WIN32)
    unsigned long ul = 1;
    int ret = ::ioctlsocket(_fd, FIONBIO, &ul); //设置为非阻塞模式
#else
    int ul = 1;
    int ret = ::ioctl(_fd, FIONBIO, &ul); //设置为非阻塞模式
#endif //defined(_WIN32)
    if (ret == -1) {
        logWarn << "ioctl FIONBIO failed: " << strerror(errno);
    }

    return ret;
}

int Socket::setNoDelay()
{
    int opt = 1;
    int ret = setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret == -1) {
        logWarn << "setsockopt TCP_NODELAY failed: " << strerror(errno);
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
        logWarn << "setsockopt SO_SNDBUF failed: " << strerror(errno);
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
        logWarn << "setsockopt SO_RCVBUF failed: " << strerror(errno);
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
#ifndef _WIN32
        logWarn << "setsockopt SO_LINGER failed: " << strerror(errno);
#endif
    }
    return ret;
}

int Socket::setCloExec()
{
#if !defined(_WIN32)
    int on = 1;
    int flags = fcntl(_fd, F_GETFD);
    if (flags == -1) {
        logWarn << "fcntl F_GETFD failed: " << strerror(errno);
        return -1;
    }
    if (on) {
        flags |= FD_CLOEXEC;
    }
    else {
        int cloexec = FD_CLOEXEC;
        flags &= ~cloexec;
    }
    int ret = fcntl(_fd, F_SETFD, flags);
    if (ret == -1) {
        logWarn << "fcntl F_SETFD failed: " << strerror(errno);
        return -1;
    }
    return ret;
#else
    return -1;
#endif
}

int Socket::setKeepAlive(int interval, int idle, int times)
{
    // Enable/disable the keep-alive option
    int on = 1;
    int opt = 1;
    int ret = setsockopt(_fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret == -1) {
        logWarn << "setsockopt SO_KEEPALIVE failed: " << strerror(errno);
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
            logWarn << "setsockopt TCP_KEEPIDLE failed: "  << strerror(errno);
        }
        ret = setsockopt(_fd, SOL_TCP, TCP_KEEPINTVL, (char *) &interval, static_cast<socklen_t>(sizeof(interval)));
        if (ret == -1) {
            logWarn << "setsockopt TCP_KEEPINTVL failed: " << strerror(errno);
        }
        ret = setsockopt(_fd, SOL_TCP, TCP_KEEPCNT, (char *) &times, static_cast<socklen_t>(sizeof(times)));
        if (ret == -1) {
            logWarn << "setsockopt TCP_KEEPCNT failed: " << strerror(errno);
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
        logInfo << "bind socket failed, ip: " << localIp << ", port: " << port << ", err: " << strerror(errno);
        return -1;
    }

    return 0;
}

int Socket::listen(int backlog)
{
    //开始监听
    if (::listen(_fd, backlog) == -1) {
        logError << "Listen socket failed: " << strerror(errno);
        close();
        return -1;
    }

    return 0;
}

int Socket::connect(const string& peerIp, int port, int timeout)
{
    _isClient = true;
    sockaddr_storage addr;
    int netType = getNetType(peerIp);
    int ss_family = AF_INET;
    if (netType == NET_IPV6) {
        ss_family = AF_INET6;
    }
    //优先使用ipv4地址
    if (!DnsCache::instance().getDomainIP(peerIp.data(), port, addr, ss_family, SOCK_STREAM, IPPROTO_TCP)) {
        //dns解析失败
        return -1;
    }
#ifndef _WIN32
    struct timeval timeo = {3, 0};
    timeo.tv_sec = timeout;
    setsockopt(_fd, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));
#else
    // 设置发送超时时间，单位为毫秒
    int sendTimeout = 3000;

    // 使用 setsockopt 函数设置发送超时选项
    if (setsockopt(_fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&sendTimeout, sizeof(sendTimeout)) == SOCKET_ERROR) {
        std::cerr << "setsockopt failed: " << WSAGetLastError() << std::endl;

        return -1;
    }
#endif
    int socketLen = 0;
    if (addr.ss_family == AF_INET) {
        ((sockaddr_in *) &addr)->sin_port = htons(port);
        socketLen = sizeof(sockaddr_in);
    } else {
        ((sockaddr_in6 *) &addr)->sin6_port = htons(port);
        socketLen = sizeof(sockaddr_in6);
    }

    errno = 0;
    if (::connect(_fd, (sockaddr *) &addr, socketLen) == 0) {
        addToEpoll();
        //同步连接成功
        return 0;
    }
#if defined(_WIN32)
    int errorCode = WSAGetLastError();
    logTrace << "fd: " << _fd << ", connect errno: " << errorCode;
    if(errorCode == WSAEWOULDBLOCK)
#else
    logTrace << "fd: " << _fd << ", connect errno: " << errno;

    if (errno == 115) 
#endif

    {
        addToEpoll();
        weak_ptr<Socket> wSelf = shared_from_this();
        _loop->addTimerTask(timeout * 1000, [wSelf, peerIp, port](){
            // logInfo << "time task start";
            auto self = wSelf.lock();
            if (!self) {
                return 0;
            }
            logTrace << "fd: " << self->_fd << ", time task, connect status: " << self->_isConnected;
            if (self->_isConnected) {
                return 0;
            }

            self->onError("connect timeout, peer ip: " + peerIp + ", port: " + to_string(port));
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
        //logInfo << "handle read: " << this << "fd:" << _fd;
        onRead(args);
    } 
    if (event & EPOLLOUT) {
        logTrace << "handle write: " << this << "fd" << _fd;
        if (!_isConnected) {
            _isConnected = true;
        }
        onWrite(args);
    } 
    if (event & EPOLLERR || event & EPOLLHUP) {
        // logInfo << "handle error";
        onError(strerror(errno), args);
    }
}

void Socket::addToEpoll()
{
    Socket::Wptr weakSocket = shared_from_this();
#ifndef _WIN32
    _loop->addEvent(_fd, EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLOUT | EPOLLET, [weakSocket](int event, void* args)
#else
    _loop->addEvent(_fd, EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLOUT, [weakSocket](int event, void* args)
#endif
       {
        auto socket = weakSocket.lock();
        if (!socket) {
            return ;
        }
        socket->handleEvent(event, args);
    });

}

thread_local StreamBuffer::Ptr g_readBuffer;
// thread_local StreamBuffer::Ptr g_writeBuffer;
StreamBuffer::Ptr Socket::onGetRecvBuffer()
{
    if (_onGetRecvBuffer) {
        return _onGetRecvBuffer();
    } else {
        return g_readBuffer;
    }
}

int Socket::onRead(void* args)
{
    if (!g_readBuffer) {
        g_readBuffer = StreamBuffer::create();
        g_readBuffer->setCapacity(1 + 4 * 1024 * 1024);
    }
    ssize_t ret = 0, nread = 0;

    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    while (true) {
        auto readBuffer = onGetRecvBuffer();
        if (!readBuffer) {
            return 0;
        }
        auto data = readBuffer->data();
        // 最后一个字节设置为'\0'
        auto capacity = readBuffer->getCapacity() - 1;
#if defined(_WIN32)
        int errorCode;
        do {
            nread = recvfrom(_fd, data, capacity, 0, (struct sockaddr*)&addr, &len);
            if (nread == SOCKET_ERROR) {
                errorCode = WSAGetLastError();
                if (errorCode != WSAEINTR) {
                    break;
                }
            }
        } while (-1 == nread && errorCode == WSAEINTR);
#else
        do {
            nread = recvfrom(_fd, data, capacity, 0, (struct sockaddr*)&addr, &len);
        } while (-1 == nread && EINTR == errno);
#endif
        if (nread == 0) {
            if (_type == 1) {
                onError("end of file");
            } else {
                logInfo << "Recv eof on udp socket[" << _fd << "]";
            }
            return ret;
        }

        if (nread == -1) {
#if defined(_WIN32)
            if(errorCode != WSAEWOULDBLOCK)
#else
            if (errno != EAGAIN)
#endif
            {

                if (_type == 1) {
                    logDebug << errno;
                } else {
                    logWarn << "Recv err on udp socket[" << _fd << "]: " << strerror(errno);
                }
            }
            return ret;
        }

        ret += nread;
        data[nread] = '\0';
        // 设置buffer有效数据大小
        readBuffer->setSize(nread);

        // 触发回调
        // LOCK_GUARD(_mtx_event);
        try {
            // 此处捕获异常，目的是防止数据未读尽，epoll边沿触发失效的问题
            _onRead(readBuffer, (struct sockaddr *)&addr, len);
        } catch (std::exception &ex) {
            logInfo << "Exception occurred when emit on_read: " << ex.what();
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

    if (_readyBuffer.size() == 0) {
        if (!_sendBuffer) {
            return 0;
        } else if (_sendBuffer->length == 0) {
            logTrace << "no buffer to send";
            _loop->modifyEvent(_fd, EPOLLIN | EPOLLHUP | EPOLLERR | 0, nullptr);
            return 0;
        }
    }

    send(nullptr);

    return 0;
}

int Socket::onError(const std::string& errMsg, void* args)
{
    // logInfo << "get a socket err: " << strerror(errno);
    weak_ptr<Socket> weakSelf = shared_from_this();
    _loop->async([weakSelf, errMsg](){
        auto self = weakSelf.lock();
        if (!self) {
            return ;
        }
        self->_sendBuffer = nullptr;
        self->_readyBuffer.clear();
        // self->close();
        if (self->_onError) {
            self->_onError(errMsg);
        }
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
#if defined(_WIN32)
        closesocket(_fd);
#else
        ::close(_fd);
#endif
        _fd = 0;
    }

    return 0;
}

ssize_t Socket::send(const char* data, int len, int flag, struct sockaddr *addr, socklen_t addr_len)
{
    auto buffer = make_shared<StreamBuffer>(data, len);
    return send(buffer, flag, 0, 0, addr, addr_len);
}

ssize_t Socket::send(const Buffer::Ptr pkt, int flag, int offset, int length, struct sockaddr *addr, socklen_t addr_len)
{
    if (!_sendBuffer) {
        return 0;
    }
    
    if (!_loop->isCurrent()) { //切换到当前socket线程发送
        Socket::Wptr wSelf = shared_from_this();
        _loop->async([wSelf, pkt, flag, offset, length, addr, addr_len](){
            auto self = wSelf.lock();
            if (self) {
                self->send(pkt, flag, offset, length, addr, addr_len);
            }
        }, true, true);
        logInfo << "change thread";
        return 0;
    }
    // 需要改为配置读取
    // 超过缓存了，丢掉pkt
    // 做丢包处理，按整包丢，避免tcp数据错位
    bool dropFlag = flag;
    bool originDrop = _drop;
    if (_drop) {
        // pkt = nullptr;
    } else if (_remainSize > 10 * 1024 * 1024) {
        // if (flag && _sendBuffer->length > 0) {
        //     // _readyBuffer.push_back(_sendBuffer);
        //     // _sendBuffer = make_shared<SocketBuffer>();
        // } else {
        //     // pkt = nullptr;
        //     // flag = false;
        // }
        if (_sendBuffer->length > 0) {
            _sendBuffer = make_shared<SocketBuffer>();
        }
        _drop = true;
        originDrop = _drop;
        if (flag) {
            dropFlag = false;
        }
        logWarn << "overlow buffer: " << _remainSize;
        logInfo << "dropFlag: " << dropFlag;
        logInfo << "originDrop: " << originDrop;
        logInfo << "_drop: " << _drop;
        logInfo << "flag: " << flag;
    }

    // 已经过了一个整包，重新判断是否要丢包
    if (dropFlag) {
        _drop = false;
    }

    if (!originDrop) {
        if (pkt && pkt->size() > 0) {
            // logInfo << "send pkt size: " << pkt->size() << ", flag : " << flag;
            SocketBuf io;
            int size = length ? length : (pkt->size() - offset);
#ifndef _WIN32
            io.iov_base = pkt->data() + offset;
            io.iov_len = size;
#else
            io.buf = pkt->data() + offset;
            io.len = size;
#endif

            _sendBuffer->vecBuffer.emplace_back(std::move(io));
            _sendBuffer->rawBuffer.push_back(pkt);
            _sendBuffer->length += size;
            _sendBuffer->socket = shared_from_this();
            _sendBuffer->addr = addr;
            _sendBuffer->addr_len = addr_len;

            _remainSize += size;
        } else {
            if (pkt) {
                logTrace << "pkt size: " << 0 << ", flag : " << flag;
            } else {
                logTrace << "pkt empty, flag : " << flag;
            }
            // logInfo << "send pkt size: " << 0 << ", flag : " << flag;
        }
    }

    if (flag || _sendBuffer->vecBuffer.size() > 1000) {
        if (_sendBuffer->length > 0) {
            _readyBuffer.push_back(_sendBuffer);
        }
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
            logTrace << "sendBuffer->length is 0";
            _readyBuffer.pop_front();
            continue;
        }

        ssize_t sendSize = 0;

#if defined(_WIN32)

        do {
            // 准备分散缓冲区数组（WSABUF 数组与原代码 msg.msg_iov 对应）
            std::vector<WSABUF>& vecBuffer = sendBuffer->vecBuffer;
            DWORD bytesSent = 0;
            int flags = 0;

            // Windows 下 MSG_DONTWAIT 对应非阻塞发送，需先将套接字设为非阻塞（或使用异步选择）
            // MSG_NOSIGNAL 在 Windows 中无等效（Windows 不会发送 SIGPIPE）
            // 这里显式添加 MSG_DONTWAIT 标志（需确保套接字支持，否则可能返回 WSAEWOULDBLOCK）
            //flags |= MSG_DONTWAIT;

            // 调用 WSASendTo 发送分散缓冲区数据
            int result = WSASendTo(
                _fd,                          // 套接字
                &(sendBuffer->vecBuffer[0]),           // WSABUF 数组指针（对应 msg.msg_iov）
                sendBuffer->vecBuffer.size(), // 缓冲区数量（对应 msg.msg_iovlen）
                &bytesSent,                   // 实际发送字节数
                flags,                        // 标志位
                sendBuffer->addr,             // 目标地址（对应 msg.msg_name）
                sendBuffer->addr_len,         // 地址长度（对应 msg.msg_namelen）
                nullptr,                      // 重叠结构（异步时使用）
                nullptr                       // 完成例程
            );

            if (result == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err == WSAEINTR) {        // 处理被信号中断的情况（对应 Linux EINTR）
                    continue;                 // 重试循环
                }
                else {
                    sendSize = SOCKET_ERROR;  // 其他错误，记录错误码
                    break;
                }
            }
            else {
                sendSize = static_cast<int>(bytesSent); // 成功发送的字节数
            }
        } while (sendSize == SOCKET_ERROR && WSAGetLastError() == WSAEINTR);
 /*       int erro = SOCKET_ERROR;
        do {
            DWORD sent = 0;
            erro = WSASend(_fd, const_cast<LPWSABUF>(&(sendBuffer->vecBuffer[0])),
                sendBuffer->vecBuffer.size(), &sent, 0, nullptr,nullptr);
            int errorCode = WSAGetLastError();
            sendSize = sent;
            if (errorCode != WSAEWOULDBLOCK && errorCode != WSAEINTR) break;
        }while((erro == SOCKET_ERROR) && (WSAGetLastError() == WSAEINTR));*/
#else

        do {
            int sendFlag = MSG_DONTWAIT | MSG_NOSIGNAL;
            struct msghdr msg = {0};
            msg.msg_iov = &(sendBuffer->vecBuffer[0]);
            msg.msg_iovlen = sendBuffer->vecBuffer.size();
            msg.msg_name = sendBuffer->addr;
            msg.msg_namelen = sendBuffer->addr_len;

#if defined(MSG_ZEROCOPY) &&	\
    defined(SO_ZEROCOPY) &&	\
    defined(SOL_SOCKET) && 0
            msg.msg_flags |= MSG_ZEROCOPY;
            sendFlag |= MSG_ZEROCOPY;
#endif

            sendSize = sendmsg(_fd, &msg, sendFlag);
        } while (sendSize == -1 && errno == EINTR);
#endif
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
#if defined(_WIN32)
                size_t size = it->len;
                if (sendSize >= size) {
                    sendSize -= size;
                    it = sendBuffer->vecBuffer.erase(it);
                    // sendBuffer->rawBuffer.pop_front();
                    sendBuffer->length -= size;
                }
                else {
                    it->buf = (char*)(it->buf) + sendSize;
                    it->len -= sendSize;
                    sendBuffer->length -= sendSize;
                    break;
                }
#else
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
#endif
            }
            logTrace << "sendBuffer->length: " << sendBuffer->length;
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

// ssize_t Socket::sendZeroCopy(const Buffer::Ptr pkt, int flag, int offset, int length, struct sockaddr *addr, socklen_t addr_len)
ssize_t Socket::sendZeroCopy(const Buffer::Ptr pkt, int flag, int offset, int length, struct sockaddr *addr, socklen_t addr_len)
{
#ifdef _WIN32
    return 0;
#else
    // static int pipeFd[2] = {0};
    if (_pipeFd[0] == 0) {
        pipe2(_pipeFd, O_NONBLOCK);
        fcntl(_pipeFd[0], F_SETPIPE_SZ, 65536);
        fcntl(_pipeFd[1], F_SETPIPE_SZ, 65536);
    }

    if (!_sendBuffer) {
        return 0;
    }
    
    if (!_loop->isCurrent()) { //切换到当前socket线程发送
        Socket::Wptr wSelf = shared_from_this();
        _loop->async([wSelf, pkt, flag, offset, length, addr, addr_len](){
            auto self = wSelf.lock();
            if (self) {
                self->send(pkt, flag, offset, length, addr, addr_len);
            }
        }, true, true);
        logInfo << "change thread";
        return 0;
    }
    // 需要改为配置读取
    // 超过缓存了，丢掉pkt
    // 做丢包处理，按整包丢，避免tcp数据错位
    bool dropFlag = flag;
    bool originDrop = _drop;
    if (_drop) {
        // pkt = nullptr;
    } else if (_remainSize > 10 * 1024 * 1024) {
        // if (flag && _sendBuffer->length > 0) {
        //     // _readyBuffer.push_back(_sendBuffer);
        //     // _sendBuffer = make_shared<SocketBuffer>();
        // } else {
        //     // pkt = nullptr;
        //     // flag = false;
        // }
        if (_sendBuffer->length > 0) {
            _sendBuffer = make_shared<SocketBuffer>();
        }
        _drop = true;
        if (flag) {
            dropFlag = false;
        }
        logTrace << "overlow buffer: " << _remainSize;
    }

    // 已经过了一个整包，重新判断是否要丢包
    if (dropFlag) {
        _drop = false;
    }

    if (!originDrop) {
        if (pkt && pkt->size() > 0) {
            // logInfo << "send pkt size: " << pkt->size() << ", flag : " << flag;
            iovec io;
            int size = length ? length : (pkt->size() - offset);
            io.iov_base = pkt->data() + offset;
            io.iov_len = size;

            _sendBuffer->vecBuffer.emplace_back(std::move(io));
            _sendBuffer->rawBuffer.push_back(pkt);
            _sendBuffer->length += size;
            _sendBuffer->socket = shared_from_this();
            _sendBuffer->addr = addr;
            _sendBuffer->addr_len = addr_len;

            _remainSize += size;
        } else {
            if (pkt) {
                logTrace << "pkt size: " << 0 << ", flag : " << flag;
            } else {
                logTrace << "pkt empty, flag : " << flag;
            }
            // logInfo << "send pkt size: " << 0 << ", flag : " << flag;
        }
    }

    if (flag || _sendBuffer->vecBuffer.size() > 1000) {
        if (_sendBuffer->length > 0) {
            _readyBuffer.push_back(_sendBuffer);
        }
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
            logTrace << "sendBuffer->length is 0";
            _readyBuffer.pop_front();
            continue;
        }

        ssize_t sendSize = 0;
        do {
            // struct msghdr msg = {0};
            // msg.msg_iov = &(sendBuffer->vecBuffer[0]);
            // msg.msg_iovlen = sendBuffer->vecBuffer.size();
            // msg.msg_name = sendBuffer->addr;
            // msg.msg_namelen = sendBuffer->addr_len;
            
            // sendSize = sendmsg(_fd, &msg, MSG_DONTWAIT | MSG_NOSIGNAL);

            sendSize = vmsplice(_pipeFd[1], &(sendBuffer->vecBuffer[0]), 1, SPLICE_F_NONBLOCK); // 使用SPLICE_F_MORE表示还有更多数据要写。
            if (sendSize == -1 && errno == EAGAIN) {
                logError << "vmsplice error: " << strerror(errno);
                continue;
            } else if (sendSize == -1) {
                logError << "vmsplice error: " << strerror(errno);
                break;
            } else if (sendSize > 0) {
                logInfo << "vmsplice sendSize: " << sendSize;
                logInfo << "vmsplice sendSize: " << sendSize;
                ssize_t totalSpliced = 0;
                // 修复1：循环splice直到所有数据被发送或发生错误
                while (totalSpliced < sendSize) {
                    ssize_t currentSpliced = splice(_pipeFd[0], NULL, _fd, NULL,
                                                sendSize - totalSpliced, SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
                    if (currentSpliced == -1) {
                        if (errno == EINTR) continue;
                        if (errno == EAGAIN) {
                            // 修复2：添加短暂延迟避免CPU空转
                            // usleep(100);
                            continue;
                        }
                        logError << "splice error: " << strerror(errno);
                        break;
                    }
                    totalSpliced += currentSpliced;
                    logInfo << "vmsplice totalSpliced: " << totalSpliced;
                }
                sendSize = totalSpliced;
                logInfo << "vmsplice sendSize: " << sendSize;
            }
        } while (sendSize == -1 && errno == EINTR);

        // logInfo << "sendBuffer->length: " << sendBuffer->length;
        logInfo << "sendSize: " << sendSize;
        // logInfo << "sendBuffer->vecBuffer[0].iov_base: " << (char*)sendBuffer->vecBuffer[0].iov_base;
        logInfo << "sendBuffer->vecBuffer[0].iov_len: " << sendBuffer->vecBuffer[0].iov_len;

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
            logTrace << "sendBuffer->length: " << sendBuffer->length;
            break;
        } else {
            break;
        }
    }

    _remainSize -= totalSendSize;
    logInfo << "_remainSize: " << _remainSize;
    logInfo << "totalSendSize: " << totalSendSize;

    if (_remainSize > 0) {
        _loop->modifyEvent(_fd, EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR | 0, nullptr);
    } else {
        // 上层根据实际情况，是发后面的buffer，还是断开链接
        // 可能会造成递归问题
        onGetBuffer();
    }

    return totalSendSize;
#endif
}

void Socket::getLocalInfo()
{
    if (_family == AF_INET) {
        struct sockaddr_in localaddr;
        socklen_t len = sizeof(localaddr);
        int ret = getsockname(_fd, (struct sockaddr*)&localaddr, &len);
        if (ret != 0) {
            logWarn << "get sock name failed";
            return;
        } else {
            char buf[INET_ADDRSTRLEN] = "";
            _localPort = ntohs(localaddr.sin_port);
            _localIp = inet_ntop(_family, &(localaddr.sin_addr), buf, INET_ADDRSTRLEN);
            _localIp.assign(buf);
        }
    } else {
        struct sockaddr_in6 localaddr;
        socklen_t len = sizeof(localaddr);
        int ret = getsockname(_fd, (struct sockaddr*)&localaddr, &len);
        if (ret != 0) {
            logWarn << "get sock name failed";
            return ;
        } else {
            char buf[INET6_ADDRSTRLEN] = "";
            _localPort = ntohs(localaddr.sin6_port);
            _localIp = inet_ntop(_family, &(localaddr.sin6_addr), buf, INET6_ADDRSTRLEN);
            _localIp.assign(buf);
        }
    }
}

int Socket::getLocalPort() {
    if (_localPort != -1) {
        return _localPort;
    }

    getLocalInfo();

    return _localPort;
}

string Socket::getLocalIp() {
    if (!_localIp.empty()) {
        return _localIp;
    }

    getLocalInfo();

    return _localIp;
}

void Socket::getPeerInfo()
{
    if (_family == AF_INET) {
        struct sockaddr_in peerAddr;
        socklen_t len = sizeof(peerAddr);
        int ret = getpeername(_fd, (struct sockaddr*)&peerAddr, &len);
        if (ret != 0) {
            logWarn << "get sock name failed";
            return ;
        } else {
            char buf[INET_ADDRSTRLEN] = "";
            _peerPort = ntohs(peerAddr.sin_port);
            _peerIp = inet_ntop(_family, &(peerAddr.sin_addr), buf, INET_ADDRSTRLEN);
            _peerIp.assign(buf);
        }
    } else {
        struct sockaddr_in6 peerAddr;
        socklen_t len = sizeof(peerAddr);
        int ret = getpeername(_fd, (struct sockaddr*)&peerAddr, &len);
        if (ret != 0) {
            logWarn << "get sock name failed";
            return ;
        } else {
            char buf[INET6_ADDRSTRLEN] = "";
            _peerPort = ntohs(peerAddr.sin6_port);
            _peerIp = inet_ntop(_family, &(peerAddr.sin6_addr), buf, INET6_ADDRSTRLEN);
            _peerIp.assign(buf);
        }
    }
}

int Socket::getPeerPort() {
    if (_peerPort != -1) {
        return _peerPort;
    }

    getPeerInfo();

    return _peerPort;
}

string Socket::getPeerIp() {
    if (!_peerIp.empty()) {
        return _peerIp;
    }

    getPeerInfo();

    return _peerIp;
}

sockaddr_in* Socket::getPeerAddr4()
{
    socklen_t len = sizeof(_peerAddr4);
    int ret = getpeername(_fd, (struct sockaddr*)&_peerAddr4, &len);
    if (ret != 0) {
        logWarn << "get sock name failed";
        return nullptr;
    }

    return &_peerAddr4;
}

sockaddr_in6* Socket::getPeerAddr6()
{
    socklen_t len = sizeof(_peerAddr6);
    int ret = getpeername(_fd, (struct sockaddr*)&_peerAddr6, &len);
    if (ret != 0) {
        logWarn << "get sock name failed";
        return nullptr;
    }

    return &_peerAddr6;
}

#if defined(_WIN32)
bool isTcpSocket(SOCKET sock) {
    int optval;
    int optlen = sizeof(optval);

    // 尝试获取 TCP 选项
    if (getsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, &optlen) == 0) {
        return true;
    }

    // 尝试获取 UDP 选项
    if (getsockopt(sock, IPPROTO_UDP, UDP_CHECKSUM_COVERAGE, (char*)&optval, &optlen) == 0) {
        return false;
    }

    // 无法确定类型
    return false;
}
#endif



// 辅助函数，从 sockaddr_storage 中获取 IP 地址和端口号
std::pair<std::string, uint16_t> Socket::getIpAndPort(const struct sockaddr_storage *addr)
{
    std::string ip;
    uint16_t port = 0;

    if (addr->ss_family == AF_INET) {
        // IPv4 处理
        struct sockaddr_in *ipv4 = reinterpret_cast<struct sockaddr_in*>(const_cast<struct sockaddr_storage*>(addr));
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(ipv4->sin_addr), ipStr, INET_ADDRSTRLEN);
        ip = ipStr;
        port = ntohs(ipv4->sin_port);
    } else if (addr->ss_family == AF_INET6) {
        // IPv6 处理
        struct sockaddr_in6 *ipv6 = reinterpret_cast<struct sockaddr_in6*>(const_cast<struct sockaddr_storage*>(addr));
        char ipStr[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(ipv6->sin6_addr), ipStr, INET6_ADDRSTRLEN);
        ip = ipStr;
        port = ntohs(ipv6->sin6_port);
    }

    return {ip, port};
}

static bool isInterfaceUp(const std::string& ifname) {
#ifdef _WIN32
    return false;
#else
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return false;

    ifreq ifr;
    strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    bool up = false;
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
        up = (ifr.ifr_flags & IFF_UP) != 0;
    }

    close(sock);
    return up;
#endif
}

std::vector<InterfaceInfo> Socket::getIfaceList(const std::string& ifacePrefix)
{
    std::vector<InterfaceInfo> interfaces;
#ifdef _WIN32
    return interfaces;
#else
    ifaddrs *ifap = nullptr, *ifa = nullptr;

    // 获取所有网络接口
    if (getifaddrs(&ifap) == -1) {
        logError << "获取网络接口失败: " << strerror(errno);
        return interfaces;
    }

    // 遍历网络接口
    for (ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;

        // 查找接口是否已存在
        auto it = std::find_if(interfaces.begin(), interfaces.end(),
            [&](const InterfaceInfo& info) { return info.name == ifa->ifa_name; });

        InterfaceInfo* if_info = nullptr;
        if (it == interfaces.end()) {
            InterfaceInfo info;
            info.name = ifa->ifa_name;
            info.is_up = isInterfaceUp(ifa->ifa_name);
            interfaces.push_back(info);
            if_info = &interfaces.back();
        } else {
            if_info = &(*it);
        }

        // 获取IP地址
        char ip_str[INET6_ADDRSTRLEN] = {0};
        if (ifa->ifa_addr->sa_family == AF_INET) {
            // IPv4地址
            sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
            inet_ntop(AF_INET, &addr->sin_addr, ip_str, sizeof(ip_str));
            if_info->ip = ip_str;
            if_info->net_type = NET_IPV4;
        } else if (ifa->ifa_addr->sa_family == AF_INET6) {
            // IPv6地址
            sockaddr_in6* addr = reinterpret_cast<sockaddr_in6*>(ifa->ifa_addr);
            inet_ntop(AF_INET6, &addr->sin6_addr, ip_str, sizeof(ip_str));
            if_info->ip = ip_str;
            if_info->net_type = NET_IPV6;
        }
    }

    freeifaddrs(ifap);
    return interfaces;
#endif
}

int Socket::getSocketType()
{
    int protocol;
    socklen_t protocolLength = sizeof(protocol);
 
#if defined(_WIN32)
    isTcpSocket(_fd) ? (protocol = SOCKET_TCP) : (protocol = SOCKET_UDP);
    return protocol;
#else
    // 获取socket的传输协议
    if (getsockopt(_fd, SOL_SOCKET, SO_PROTOCOL, &protocol, &protocolLength) == -1) {
        logError << "Error getting socket protocol" << std::endl;
        return -1;
    }
    return protocol == IPPROTO_TCP ? SOCKET_TCP : SOCKET_UDP;
#endif
    
}

NetType Socket::getNetType(const string& ip)
{
    struct in_addr addr;
    struct in6_addr addr6;
    if (inet_pton(AF_INET, ip.c_str(), &addr) == 1) {
        return NET_IPV4;
    } else if (inet_pton(AF_INET6, ip.c_str(), &addr6) == 1) {
        return NET_IPV6;
    } else {
        return NET_INVALID;
    }
}
