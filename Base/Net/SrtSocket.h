#ifndef SrtSocket_h
#define SrtSocket_h

#ifdef ENABLE_SRT

#include "EventPoller/SrtEventLoop.h"
#include "Buffer.h"
#include "srt/srt.h"

#include <deque>
#include <memory>
#include <netinet/in.h>

using namespace std;

class SrtScoketBuffer
{
public:
    using Ptr = shared_ptr<SrtScoketBuffer>;

    Buffer::Ptr _buffer;
    int _offset = 0;
};

class SrtSocket : public std::enable_shared_from_this<SrtSocket> {
public:
    using Ptr = shared_ptr<SrtSocket>;
    using Wptr = weak_ptr<SrtSocket>;
    using onReadCb = function<int(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)>;
    using onWriteCb = function<void()>;
    using onErrorCb = function<void()>;
    SrtSocket(const SrtEventLoop::Ptr& loop);
    SrtSocket(const SrtEventLoop::Ptr& loop, bool isListen);
    SrtSocket(const SrtEventLoop::Ptr& loop, int fd);
    ~SrtSocket();

public:
    static void initSrt();
    static void uninitSrt();

public:
    // type: 1:tcp, 2:udp, 3:quic, 4:srt
    // family ipv4 or ipv6
    sockaddr_storage createSocket(const string& peerIp, int peerPort, int type);
    int createSocket(int type);

    int setsockopt(int fd, int level, SRT_SOCKOPT optname, const void * optval, int optlen);
    int getsockopt(SRT_SOCKOPT optname, const char * optnamestr, void * optval, int * optlen);

    int epollCreate(int write);
    int networkWaitFd(int eid, int write);
    int networkWaitFdTimeout(int eid, int write, int64_t timeout);
    int setReuseable();
    int setNoSigpipe();
    int setNoBlocked(int enable);
    int setSendBuf(int size = 1024 * 1024);
    int setRecvBuf(int size = 1024 * 1024);
    int setCloseWait(int second = 0);
    int setCloExec();
    int bind(const uint16_t port, const char *localIp);
    int listen(int backlog);
    int accept();
    int connect(const string& peetIp, int port, int timeout = 5);

    int getFd() {return _fd;}
    int getLocalPort();
    string getLocalIp();
    
    int getPeerPort();
    string getPeerIp();

    void handleEvent(int event, void* args);
    void addToEpoll();

    int onRead(void* args);
    int onWrite(void* args);
    int onError(void* args);
    int onAccept(void* args);
    int close();

    ssize_t send(const char* data, int len, int flag = true, struct sockaddr *addr = nullptr, socklen_t addr_len = 0);
    ssize_t send(const Buffer::Ptr pkt, int flag = true, int offset = 0, struct sockaddr *addr = nullptr, socklen_t addr_len = 0);

    void setReadCb(const onReadCb& cb) { _onRead = cb;}
    void setWriteCb(const onWriteCb& cb) { _onWrite = cb;}
    void setErrorCb(const onErrorCb& cb) { _onError = cb;}
    void setAcceptCb(const onWriteCb& cb) { _onAccept = cb;}

    SrtEventLoop::Ptr getLoop() {return _loop;}
    int setOptionsPost();

private:
    static bool _srtInited;
    bool _isListen = false;
    bool _isClient = false;
    bool _isConnected = false;

    int _inputbw = -1;
    int _oheadbw = -1;
    uint64_t _maxbw = 0;
    int _pbkeylen = -1;
    char* _passphrase = nullptr;
    int _reuse = 1;
    int _enforced_encryption = -1;
    int _kmrefreshrate = -1;
    int _kmpreannounce = -1;
    uint64_t _snddropdelay = 0;
    uint64_t _mss = 0;
    uint64_t _ffs = 0;
    uint64_t _ipttl = 0;
    uint64_t _iptos = 0;
    uint64_t _latency = 0;
    uint64_t _rcvlatency = 0;
    uint64_t _peerlatency = 0;
    uint64_t _tlpktdrop = 0;
    uint64_t _nakreport = 0;
    uint64_t _connect_timeout  = 0;
    uint64_t _sndbuf  = 0;
    uint64_t _rcvbuf  = 0;
    uint64_t _lossmaxttl  = 0;
    uint64_t _minversion  = 0;
    uint64_t _streamid  = 0;
    uint64_t _smoother  = 0;
    uint64_t _messageapi  = 0;
    uint64_t _payload_size   = 0;
    uint64_t _flags   = 0;
    uint64_t _tsbpd   = 0;
    uint64_t _linger = 0;

    int _fd = -1;
    int _sockFd = -1;
    int _family;
    int _type = 1;
    int _localPort = -1;
    int _peerPort = -1;
    size_t _remainSize = 0;
    string _localIp;
    string _peerIp;
    list<SrtScoketBuffer::Ptr> _readyBuffer;
    SrtEventLoop::Ptr _loop;
    onReadCb _onRead;
    onWriteCb _onWrite;
    onWriteCb _onAccept;
    onErrorCb _onError;
};

#endif

#endif //Socket_h