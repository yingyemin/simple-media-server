#ifndef Socket_h
#define Socket_h

#include "EventPoller/EventLoop.h"
#include "Buffer.h"

#include <deque>
#include <memory>
#include "Util/Util.h"

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment(lib,"Iphlpapi.lib")
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <ifaddrs.h>
#endif // defined(_WIN32)


// using namespace std;

enum SocketType {
    SOCKET_TCP = 1,
    SOCKET_UDP
};

enum NetType {
    NET_INVALID = 0,
    NET_IPV4 = 1,
    NET_IPV6
};

class InterfaceInfo {
public:
    InterfaceInfo() = default;
    ~InterfaceInfo() = default;

    std::string name;           // 网卡名称
    std::string ip; // IP地址
    bool is_up;                 // 网卡是否启用
    NetType net_type = NET_INVALID; // 网络类型
};

class Socket;

#if defined(_WIN32)
using SocketBuf = WSABUF;
#else
using SocketBuf = iovec;
#endif

class SocketBuffer
{
public:
    using Ptr = std::shared_ptr<SocketBuffer>;
    void clear()
    {
        vecBuffer.clear();
        length = 0;
    }
public:
     std::vector<SocketBuf> vecBuffer;
    std::vector<Buffer::Ptr> rawBuffer;
    int length = 0;
    // 保存一份socket，保证数据没发送完，socket不会被析构
    std::shared_ptr<Socket> socket;
    struct sockaddr *addr = nullptr;
    socklen_t addr_len = 0;
};

class Socket : public std::enable_shared_from_this<Socket> {
public:
    using Ptr = std::shared_ptr<Socket>;
    using Wptr = std::weak_ptr<Socket>;
    using onReadCb = std::function<int(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)>;
    using onWriteCb = std::function<void()>;
    using onErrorCb = std::function<void(const std::string& errMsg)>;
    Socket(const EventLoop::Ptr& loop);
    Socket(const EventLoop::Ptr& loop, int fd);
    ~Socket();

public:
    // type: 1:tcp, 2:udp, 3:quic, 4:srt
    // family ipv4 or ipv6
    sockaddr_storage createSocket(const std::string& peerIp, int peerPort, int type);
    int createSocket(int type, int family = AF_INET);

    int createTcpSocket(int family);
    int createUdpSocket(int family);

    int setReuseable();
    int setIpv6Only(bool enable);
    int setZeroCopy();
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
    int connect(const std::string& peetIp, int port, int timeout = 5);

    int getFd() {return _fd;}
    uint64_t getCreateTime() {return _createTime;}
    int getLocalPort();
    std::string getLocalIp();
    void getLocalInfo();
    
    int getPeerPort();
    std::string getPeerIp();
    void getPeerInfo();
    // sockaddr* getPeerAddr();
    sockaddr_in* getPeerAddr4();
    sockaddr_in6* getPeerAddr6();

    int getSocketType();

    void handleEvent(int event, void* args);
    void addToEpoll();

    int onRead(void* args);
    int onWrite(void* args);
    int onError(const std::string& errMsg, void* args = nullptr);
    bool onGetBuffer();
    int close();

    ssize_t send(const char* data, int len, int flag = true, struct sockaddr *addr = nullptr, socklen_t addr_len = 0);
    ssize_t send(const Buffer::Ptr pkt, int flag = true, int offset = 0, int length = 0, struct sockaddr *addr = nullptr, socklen_t addr_len = 0);

    ssize_t sendZeroCopy(const Buffer::Ptr pkt, int flag = true, int offset = 0, int length = 0, struct sockaddr *addr = nullptr, socklen_t addr_len = 0);

    void setReadCb(const onReadCb& cb) { _onRead = cb;}
    void setWriteCb(const onWriteCb& cb) { _onWrite = cb;}
    void setErrorCb(const onErrorCb& cb) { _onError = cb;}
    void setOnGetBuffer(const std::function<bool()>& cb) {_onGetBuffer = cb;}
    int getFamily() {return _family;}
    void setFamily(int family) {_family = family;}

    EventLoop::Ptr getLoop() {return _loop;}

    static NetType getNetType(const std::string& ip);

    void setOnGetRecvBuffer(const std::function<StreamBuffer::Ptr()>& cb) {_onGetRecvBuffer = cb;}
    StreamBuffer::Ptr onGetRecvBuffer();

    static std::pair<std::string, uint16_t> getIpAndPort(const struct sockaddr_storage *addr);

    static std::vector<InterfaceInfo> getIfaceList(const std::string& ifacePrefix);

private:
    bool _isClient = false;
    bool _isConnected = false;
    int _fd = 0;// windows 是unsigned int64 不确定是否能用
    bool _drop = false;
    int _family = AF_INET;
    int _type = 1;
    int _localPort = -1;
    int _peerPort = -1;
    int _pipeFd[2] = {0};
    uint64_t _createTime = 0;
    size_t _remainSize = 0;
    std::string _localIp;
    std::string _peerIp;
    sockaddr_in _peerAddr4;
    sockaddr_in6 _peerAddr6;
    SocketBuffer::Ptr _sendBuffer;
    std::deque<SocketBuffer::Ptr> _readyBuffer;
    EventLoop::Ptr _loop;
    onReadCb _onRead;
    onWriteCb _onWrite;
    onErrorCb _onError;
    std::function<bool()> _onGetBuffer;
    std::function<StreamBuffer::Ptr()> _onGetRecvBuffer;
};

#endif //Socket_h