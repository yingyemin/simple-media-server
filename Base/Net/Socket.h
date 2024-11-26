#ifndef Socket_h
#define Socket_h

#include "EventPoller/EventLoop.h"
#include "Buffer.h"

#include <deque>
#include <memory>
#include <netinet/in.h>

using namespace std;

enum SocketType {
    SOCKET_TCP = 1,
    SOCKET_UDP
};

enum NetType {
    NET_INVALID = 0,
    NET_IPV4 = 1,
    NET_IPV6
};

class Socket;

class SocketBuffer
{
public:
    using Ptr = shared_ptr<SocketBuffer>;
    void clear()
    {
        vecBuffer.clear();
        length = 0;
    }
public:
    vector<iovec> vecBuffer;
    vector<Buffer::Ptr> rawBuffer;
    int length = 0;
    // 保存一份socket，保证数据没发送完，socket不会被析构
    shared_ptr<Socket> socket;
    struct sockaddr *addr = nullptr;
    socklen_t addr_len = 0;
};

class Socket : public std::enable_shared_from_this<Socket> {
public:
    using Ptr = shared_ptr<Socket>;
    using Wptr = weak_ptr<Socket>;
    using onReadCb = function<int(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)>;
    using onWriteCb = function<void()>;
    using onErrorCb = function<void()>;
    Socket(const EventLoop::Ptr& loop);
    Socket(const EventLoop::Ptr& loop, int fd);
    ~Socket();

public:
    // type: 1:tcp, 2:udp, 3:quic, 4:srt
    // family ipv4 or ipv6
    sockaddr_storage createSocket(const string& peerIp, int peerPort, int type);
    int createSocket(int type, int family = AF_INET);

    int createTcpSocket(int family);
    int createUdpSocket(int family);

    int setReuseable();
    int setIpv6Only(bool enable);
    int setNoSigpipe();
    int setNoBlocked();
    int setNoDelay();
    int setSendBuf(int size = 1024 * 1024);
    int setRecvBuf(int size = 1024 * 1024);
    int setCloseWait(int second = 0);
    int setCloExec();
    int setKeepAlive(int interval, int idle, int times);
    int bind(const uint16_t port, const char *localIp);
    int listen(int backlog);
    bool bindPeerAddr(const struct sockaddr *dst_addr);
    int connect(const string& peetIp, int port, int timeout = 5);

    int getFd() {return _fd;}
    int getLocalPort();
    string getLocalIp();
    void getLocalInfo();
    
    int getPeerPort();
    string getPeerIp();
    void getPeerInfo();
    // sockaddr* getPeerAddr();
    sockaddr_in* getPeerAddr4();
    sockaddr_in6* getPeerAddr6();

    int getSocketType();

    void handleEvent(int event, void* args);
    void addToEpoll();

    int onRead(void* args);
    int onWrite(void* args);
    int onError(void* args);
    bool onGetBuffer();
    int close();

    ssize_t send(const char* data, int len, int flag = true, struct sockaddr *addr = nullptr, socklen_t addr_len = 0);
    ssize_t send(const Buffer::Ptr pkt, int flag = true, int offset = 0, int length = 0, struct sockaddr *addr = nullptr, socklen_t addr_len = 0);

    void setReadCb(const onReadCb& cb) { _onRead = cb;}
    void setWriteCb(const onWriteCb& cb) { _onWrite = cb;}
    void setErrorCb(const onErrorCb& cb) { _onError = cb;}
    void setOnGetBuffer(const function<bool()>& cb) {_onGetBuffer = cb;}
    int getFamily() {return _family;}
    void setFamily(int family) {_family = family;}

    EventLoop::Ptr getLoop() {return _loop;}

    static NetType getNetType(const string& ip);

    void setOnGetRecvBuffer(const function<StreamBuffer::Ptr()>& cb) {_onGetRecvBuffer = cb;}
    StreamBuffer::Ptr onGetRecvBuffer();

private:
    bool _isClient = false;
    bool _isConnected = false;
    int _fd = -1;
    int _family = AF_INET;
    int _type = 1;
    int _localPort = -1;
    int _peerPort = -1;
    size_t _remainSize = 0;
    string _localIp;
    string _peerIp;
    sockaddr_in _peerAddr4;
    sockaddr_in6 _peerAddr6;
    SocketBuffer::Ptr _sendBuffer;
    list<SocketBuffer::Ptr> _readyBuffer;
    EventLoop::Ptr _loop;
    onReadCb _onRead;
    onWriteCb _onWrite;
    onErrorCb _onError;
    function<bool()> _onGetBuffer;
    function<StreamBuffer::Ptr()> _onGetRecvBuffer;
};

#endif //Socket_h