#include "SrtSocket.h"
#include "DnsCache.h"
#include "Logger.h"
#include "Util/TimeClock.h"

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

#ifdef ENABLE_SRT

#define POLLING_TIME 1

bool SrtSocket::_srtInited = false;

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
    if (srt_bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
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
    if (srt_bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
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

static int srtErrno()
{
    int srt_errno;
    int err = srt_getlasterror(&srt_errno);

    if (err == SRT_EASYNCRCV || err == SRT_EASYNCSND) {
        return EAGAIN;
    }

    logInfo << "srt error info: " << srt_getlasterror_str();

    return srt_errno;
}

SrtSocket::SrtSocket(const SrtEventLoop::Ptr& loop)
    :_loop(loop)
{
    logInfo << "SrtSocket";
}

SrtSocket::SrtSocket(const SrtEventLoop::Ptr& loop, bool isListen)
    :_loop(loop)
    ,_isListen(isListen)
{
    logInfo << "SrtSocket";
}

SrtSocket::SrtSocket(const SrtEventLoop::Ptr& loop, int fd)
    :_loop(loop)
    ,_fd(fd)
{
    logInfo << "SrtSocket";
}

SrtSocket::~SrtSocket()
{
    logInfo << "~SrtSocket, fd: " << _fd;
    close();
}

void SrtSocket::initSrt()
{
    if (_srtInited) {
        return ;
    }

    if (srt_startup() < 0) {
        logWarn << "init srt failed";
        return ;
    }

    _srtInited = true;
}

void SrtSocket::uninitSrt()
{
    if (!_srtInited) {
        return ;
    }

    srt_cleanup();

    _srtInited = false;
}

sockaddr_storage SrtSocket::createSocket(const string& peerIp, int peerPort, int type)
{
    sockaddr_storage addr;
    //优先使用ipv4地址
    if (!DnsCache::instance().getDomainIP(peerIp.data(), peerPort, addr, AF_INET, SOCK_STREAM, IPPROTO_TCP)) {
        //dns解析失败
        return addr;
    }

    _fd = srt_create_socket();

    if (_fd < 0) {
        srtErrno();
    }

    return addr;
}



int SrtSocket::createSocket(int type)
{
    int ret = 0;

    _fd = srt_create_socket();
    if (_fd < 0) {
        ret = srtErrno();
    }

    if (_latency > 0) {
        srt_setsockopt(_fd, SOL_SOCKET, SRTO_LATENCY, &_latency, sizeof (_latency));
    }
    if (_rcvbuf > 0) {
        srt_setsockopt(_fd, SOL_SOCKET, SRTO_UDP_RCVBUF, &_rcvbuf, sizeof (_rcvbuf));
    }
    if (_sndbuf > 0) {
        srt_setsockopt(_fd, SOL_SOCKET, SRTO_UDP_SNDBUF, &_sndbuf, sizeof (_sndbuf));
    }
    if (_reuse) {
        if (srt_setsockopt(_fd, SOL_SOCKET, SRTO_REUSEADDR, &_reuse, sizeof(_reuse))) {
            logWarn << "CSLSSrt::libsrt_setup, setsockopt(SRTO_REUSEADDR) failed";
        }
        logTrace << "srt socket reuse addr";
    }
/* BEGIN: Added by msavskit, 2023/10/6 ,支持加密功能*/
	if (_pbkeylen>= 0) {
		if (srt_setsockopt(_fd, SOL_SOCKET, SRTO_PBKEYLEN, &_pbkeylen, sizeof(_pbkeylen))) {
            logWarn << "CSLSSrt::libsrt_setup, setsockopt(SRTO_PBKEYLEN) failed";
        }
	}
	if (_passphrase) {
		if (srt_setsockopt(_fd, SOL_SOCKET, SRTO_PASSPHRASE, _passphrase, strlen(_passphrase)))
            logWarn << "CSLSSrt::libsrt_setup, setsockopt(SRTO_PASSPHRASE) failed";
	}
#if SRT_VERSION_VALUE >= 0x010302
#if SRT_VERSION_VALUE >= 0x010401
	if (_enforced_encryption >= 0) {
		if (srt_setsockopt(_fd, SOL_SOCKET, SRTO_ENFORCEDENCRYPTION, &_enforced_encryption, sizeof(_enforced_encryption))) {
            logWarn << "CSLSSrt::libsrt_setup, setsockopt(SRTO_ENFORCEDENCRYPTION) failed";
        }
	}
#else
	if (_enforced_encryption >= 0) {
		if (srt_setsockopt(_fd, SOL_SOCKET, SRTO_STRICTENC, &_enforced_encryption, sizeof(_enforced_encryption))){
            logWarn << "CSLSSrt::libsrt_setup, setsockopt(SRTO_ENFORCEDENCRYPTION) failed";
        }
	}
#endif
	if (_kmrefreshrate >= 0) {
		if (srt_setsockopt(_fd, SOL_SOCKET, SRTO_KMREFRESHRATE, &_kmrefreshrate, sizeof(_kmrefreshrate))){
            logWarn << "CSLSSrt::libsrt_setup, setsockopt(SRTO_KMREFRESHRATE) failed";
        }
	}
	if (_kmpreannounce >= 0) {
		if (srt_setsockopt(_fd, SOL_SOCKET, SRTO_KMPREANNOUNCE, &_kmpreannounce, sizeof(_kmpreannounce))){
            logWarn << "CSLSSrt::libsrt_setup, setsockopt(SRTO_KMPREANNOUNCE) failed";
        }
	}
#endif

    setOptionsPost();
    setNoBlocked(true);

    return ret;
}

int SrtSocket::setsockopt(int fd, int level, SRT_SOCKOPT optname, const void * optval, int optlen)
{
    if (srt_setsockopt(fd, 0, optname, optval, optlen) < 0) {
        logError << "failed to set option " << optname << " on socket: " << srt_getlasterror_str();
        return EIO;
    }

    return 0;
}

int SrtSocket::setOptionsPost()
{
    if ((_inputbw >= 0 && setsockopt(_fd, SOL_SOCKET, SRTO_INPUTBW, &_inputbw, sizeof(_inputbw)) < 0) ||
        (_oheadbw >= 0 && setsockopt(_fd, SOL_SOCKET, SRTO_OHEADBW, &_oheadbw, sizeof(_oheadbw)) < 0)) {
        return EIO;
    }

    return 0;
}

int SrtSocket::getsockopt(SRT_SOCKOPT optname, const char * optnamestr, void * optval, int * optlen)
{
    if (srt_getsockopt(_fd, 0, optname, optval, optlen) < 0) {
        logError << "failed to get option " << optnamestr << " on socket: " << srt_getlasterror_str();
        return EIO;
    }

    return 0;
}

int SrtSocket::setNoBlocked(int enable)
{
    int ret, blocking = enable ? 0 : 1;
    /* Setting SRTO_{SND,RCV}SYN options to 1 enable blocking mode, setting them to 0 enable non-blocking mode. */
    ret = srt_setsockopt(_fd, 0, SRTO_SNDSYN, &blocking, sizeof(blocking));
    if (ret < 0) {
        logWarn << "srt_setsockopt SRTO_SNDSYN failed";
        return ret;
    }

    return srt_setsockopt(_fd, 0, SRTO_RCVSYN, &blocking, sizeof(blocking));
}

int SrtSocket::epollCreate(int write)
{
    int modes = SRT_EPOLL_ERR | (write ? SRT_EPOLL_OUT : SRT_EPOLL_IN);
    int eid = srt_epoll_create();
    if (eid < 0) {
        return srtErrno();
    }

    if (srt_epoll_add_usock(eid, _fd, &modes) < 0) {
        srt_epoll_release(eid);
        return srtErrno();
    }

    return eid;
}

int SrtSocket::networkWaitFd(int eid, int write)
{
    int ret, len = 1, errlen = 1;
    SRTSOCKET ready[1];
    SRTSOCKET error[1];

    if (write) {
        ret = srt_epoll_wait(eid, error, &errlen, ready, &len, POLLING_TIME, 0, 0, 0, 0);
    } else {
        ret = srt_epoll_wait(eid, ready, &len, error, &errlen, POLLING_TIME, 0, 0, 0, 0);
    }
    if (ret < 0) {
        if (srt_getlasterror(NULL) == SRT_ETIMEOUT)
            ret = EAGAIN;
        else
            ret = srtErrno();
    } else {
        ret = errlen ? EIO : 0;
    }

    return ret;
}

int SrtSocket::networkWaitFdTimeout(int eid, int write, int64_t timeout)
{
    int ret;
    int64_t wait_start = 0;

    while (1) {
        ret = networkWaitFd(eid, write);
        if (ret != EAGAIN) {
            return ret;
        }
        
        if (timeout > 0) {
            if (!wait_start)
                wait_start = TimeClock::now();
            else if (TimeClock::now() - wait_start > timeout)
                return ETIMEDOUT;
        }
    }
}


int SrtSocket::setReuseable()
{
    int opt = 1;
    int ret = setsockopt(_fd, SOL_SOCKET, SRTO_REUSEADDR, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    if (ret != 0) {
        logInfo << "setsockopt SRTO_REUSEADDR failed";
        return ret;
    }
    
    // ret = setsockopt(_fd, SOL_SOCKET, SRTO_REUSEPORT, (char *) &opt, static_cast<socklen_t>(sizeof(opt)));
    // if (ret != 0) {
    //     logInfo << "setsockopt SO_REUSEPORT failed";
    // }
    
    return ret;
}

int SrtSocket::setNoSigpipe()
{
#if defined(SO_NOSIGPIPE)
    int set = 1;
    auto ret = setsockopt(_fd, SOL_SOCKET, SRTO_NOSIGPIPE, (char *) &set, sizeof(int));
    if (ret == -1) {
        cout << "setsockopt SO_NOSIGPIPE failed";
    }
    return ret;
#else
    return -1;
#endif
}

int SrtSocket::setSendBuf(int size)
{
    if (size <= 0) {
        return 0;
    }
    int ret = setsockopt(_fd, SOL_SOCKET, SRTO_SNDBUF, (char *) &size, sizeof(size));
    if (ret == -1) {
        cout << "setsockopt SO_SNDBUF failed";
    }
    return ret;
}

int SrtSocket::setRecvBuf(int size)
{
    if (size <= 0) {
        // use the system default value
        return 0;
    }
    int ret = setsockopt(_fd, SOL_SOCKET, SRTO_RCVBUF, (char *) &size, sizeof(size));
    if (ret == -1) {
        cout << "setsockopt SO_RCVBUF failed";
    }
    return ret;
}

int SrtSocket::setCloseWait(int second)
{
    linger m_sLinger;
    //在调用closesocket()时还有数据未发送完，允许等待
    // 若m_sLinger.l_onoff=0;则调用closesocket()后强制关闭
    m_sLinger.l_onoff = (second > 0);
    m_sLinger.l_linger = second; //设置等待时间为x秒
    int ret = setsockopt(_fd, SOL_SOCKET, SRTO_LINGER, (char *) &m_sLinger, sizeof(linger));
    if (ret == -1) {
        cout << "setsockopt SO_LINGER failed";
    }
    return ret;
}

int SrtSocket::setCloExec()
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

int SrtSocket::bind(const uint16_t port, const char *localIp)
{
    // int family = support_ipv6() ? (is_ipv4(local_ip) ? AF_INET : AF_INET6) : AF_INET;
    int family = AF_INET;
    if (bindSock(_fd, localIp, port, AF_INET) == -1) {
        close();
        logInfo << "bind socket failed, ip: " << localIp << ", port: " << port;
        return -1;
    }

    return 0;
}

int SrtSocket::listen(int backlog)
{
    //开始监听
    if (srt_listen(_fd, 1)) {
        logInfo << "Listen socket failed: " << srtErrno();
        close();
        return -1;
    }

    return 0;
}

int SrtSocket::accept()
{
    struct sockaddr_in scl;
    int sclen = sizeof(scl);
    char ip[30] = {0};
    struct sockaddr_in * addrtmp;

    logInfo << "accept fd: " << _fd;

    int new_sock = srt_accept(_fd, (struct sockaddr*)&scl, &sclen);//NULL, NULL);//(sockaddr*)&scl, &sclen);
    if (new_sock == SRT_INVALID_SOCK) {
        int err_no = srtErrno();
        return -1;
    }

    addrtmp = (struct sockaddr_in *)&scl;
    inet_ntop(AF_INET, &addrtmp->sin_addr, ip, sizeof(ip));

    logInfo << "accept ok, new sock=" << new_sock << ", " << ip << ":" << ntohs(addrtmp->sin_port);

    return new_sock;
}

int SrtSocket::connect(const string& peerIp, int port, int timeout)
{
    _isClient = true;
    sockaddr_storage addr;
    //优先使用ipv4地址
    if (!DnsCache::instance().getDomainIP(peerIp.data(), port, addr, AF_INET, SOCK_STREAM, IPPROTO_TCP)) {
        //dns解析失败
        return -1;
    }

    // struct timeval timeo = {3, 0};
    // timeo.tv_sec = timeout;
    // setsockopt(_fd, SOL_SOCKET, SRTO_SNDTIMEO, &timeo, sizeof(timeo));
    int conn_timeout_len = sizeof(timeout);
    srt_getsockopt(_fd, 0, SRTO_CONNTIMEO, &timeout, &conn_timeout_len);

    ((sockaddr_in *) &addr)->sin_port = htons(port);
    errno = 0;
    if (srt_connect(_fd, (sockaddr *) &addr, sizeof(sockaddr_in)) >= 0) {
        addToEpoll();
        //同步连接成功
        return 0;
    }
    
    // logInfo << "errno: " << errno;
    // if (errno == 115) {
    //     addToEpoll();
    //     weak_ptr<SrtSocket> wSelf = shared_from_this();
    //     _loop->addTimerTask(timeout * 1000, [wSelf](){
    //         logInfo << "time task start";
    //         auto self = wSelf.lock();
    //         if (!self) {
    //             return 0;
    //         }
    //         logInfo << "time task connect: " << self->_isConnected;
    //         if (self->_isConnected) {
    //             return 0;
    //         }
    //         self->onError(nullptr);
    //         return 0;
    //     }, [](bool success, const shared_ptr<TimerTask>& task){});

    //     return 0;
    // }

    return -1;
}

void SrtSocket::handleEvent(int event, void* args)
{
    // logInfo << "handle event: " << event;
    if (_isListen) {
        onAccept(args);
        return ;
    }
    // logInfo << "EPOLLIN: " << EPOLLIN;
    // logInfo << "EPOLLOUT: " << EPOLLOUT;
    // logInfo << "EPOLLERR: " << EPOLLERR;
    // logInfo << "EPOLLHUP: " << EPOLLHUP;
    if (event & EPOLLIN) {
        // logInfo << "handle read";
        onRead(args);
    } 
    if (event & EPOLLOUT) {
        logInfo << "handle write";
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

void SrtSocket::addToEpoll()
{
    SrtSocket::Wptr weakSocket = shared_from_this();
    _loop->addEvent(_fd, 0, [weakSocket](int event, void* args){
        auto socket = weakSocket.lock();
        if (!socket) {
            return ;
        }
        socket->handleEvent(event, args);
    });
}

thread_local StreamBuffer::Ptr g_srtReadBuffer;
thread_local StreamBuffer::Ptr g_srtWriteBuffer;
int SrtSocket::onRead(void* args)
{
    if (!g_srtReadBuffer) {
        g_srtReadBuffer = StreamBuffer::create();
        g_srtReadBuffer->setCapacity(1 + 4 * 1024 * 1024);
    }
    ssize_t ret = 0, nread = 0;
    auto data = g_srtReadBuffer->data();
    // 最后一个字节设置为'\0'
    auto capacity = g_srtReadBuffer->getCapacity() - 1;

    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    while (true) {
        nread = srt_recvmsg(_fd, data, capacity);

        if (nread < 0) {
            auto retno = srtErrno();
            if (retno != EAGAIN) {
                close();
                onError(nullptr);
            }

            return retno;
        } else if (nread == 0) {
            break;
        }
        

        ret += nread;
        data[nread] = '\0';
        // 设置buffer有效数据大小
        g_srtReadBuffer->setSize(nread);

        // 触发回调
        // LOCK_GUARD(_mtx_event);
        try {
            // 此处捕获异常，目的是防止数据未读尽，epoll边沿触发失效的问题
            if (_onRead) {
                _onRead(g_srtReadBuffer, (struct sockaddr *)&addr, len);
            }
        } catch (std::exception &ex) {
            logError << "Exception occurred when emit on_read: " << ex.what();
        }
    }
    return 0;
}

int SrtSocket::onWrite(void* args)
{
    // if (!g_srtWriteBuffer) {
    //     g_srtWriteBuffer = StreamBuffer::create();
    //     g_srtWriteBuffer->setCapacity(1 + 4 * 1024 * 1024);
    // }
    if (_onWrite) {
        _onWrite();
    }

    if (_readyBuffer.size() == 0) {
        logInfo << "no buffer to send";
        _loop->modifyEvent(_fd, 1, nullptr);
        return 0;
    }

    send(nullptr);

    return 0;
}

int SrtSocket::onError(void* args)
{
    weak_ptr<SrtSocket> weakSelf = shared_from_this();
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

int SrtSocket::onAccept(void* args)
{
    weak_ptr<SrtSocket> weakSelf = shared_from_this();
    _loop->async([weakSelf](){
        auto self = weakSelf.lock();
        if (!self) {
            return ;
        }
        // self->close();
        if (self->_onAccept) {
            self->_onAccept();
        }
    }, true, false);

    return 0;
}

int SrtSocket::close()
{
    logInfo << "SrtSocket::close()";
    if (_fd > 0) {
        _loop->delEvent(_fd, [](bool success){});
        srt_close(_fd);
        _fd = 0;
    }

    return 0;
}

ssize_t SrtSocket::send(const char* data, int len, int flag, struct sockaddr *addr, socklen_t addr_len)
{
    auto buffer = make_shared<StreamBuffer>(data, len);
    return send(buffer, flag, 0, addr, addr_len);
}

ssize_t SrtSocket::send(const Buffer::Ptr pkt, int flag, int offset, struct sockaddr *addr, socklen_t addr_len)
{
    if (!_loop->isCurrent()) { //切换到当前socket线程发送
        SrtSocket::Wptr wSelf = shared_from_this();
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
        logInfo << "overlow buffer";
        return 0;
    }

    if (pkt) {
        auto buffer = make_shared<SrtScoketBuffer>();
        buffer->_buffer = pkt;

        _readyBuffer.push_back(buffer);

        _remainSize += pkt->size();
    } else {
        // logInfo << "send pkt size: " << 0 << ", flag : " << flag;
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
        if (!sendBuffer->_buffer || sendBuffer->_buffer->size() - sendBuffer->_offset == 0) {
            logInfo << "sendBuffer->length is 0";
            _readyBuffer.pop_front();
            continue;
        }

        int left = sendBuffer->_buffer->size() - sendBuffer->_offset;

        ssize_t sendSize = srt_sendmsg(_fd, sendBuffer->_buffer->data() + sendBuffer->_offset, left, -1, 0);

        // logInfo << "sendBuffer->length: " << sendBuffer->length;
        // logInfo << "sendSize: " << sendSize;

        if (sendSize >= left) {
            // logInfo << "sendBuffer->length: " << sendBuffer->length;
            totalSendSize += left;
            _readyBuffer.pop_front();
            continue;
        } else if (sendSize > 0) {
            totalSendSize += sendSize;
            sendBuffer->_offset += sendSize;
            logInfo << "sendBuffer->_offset: " << sendBuffer->_offset;
            break;
        } else {
            onError(nullptr);
            break;
        }
    }

    _remainSize -= totalSendSize;
    // logInfo << "_remainSize: " << _remainSize;
    // logInfo << "totalSendSize: " << totalSendSize;

    if (_remainSize > 0) {
        _loop->modifyEvent(_fd, 1, nullptr);
    }

    return totalSendSize;
}

int SrtSocket::getLocalPort() {
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

string SrtSocket::getLocalIp() {
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

int SrtSocket::getPeerPort() {
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

string SrtSocket::getPeerIp() {
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

#endif