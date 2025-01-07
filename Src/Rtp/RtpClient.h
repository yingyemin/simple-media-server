#ifndef RtpClient_h
#define RtpClient_h

#include "Net/Socket.h"
#include "Net/TcpClient.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

using namespace std;

class RtpClient : public enable_shared_from_this<RtpClient> {
public:
    using Ptr = shared_ptr<RtpClient>;
    using Wptr = weak_ptr<RtpClient>;

    RtpClient(const string& ip, int port, const string& app, 
                    const string& stream, int ssrc, int sockType, bool sendFlag);
    ~RtpClient();

public:
    // sockType: 1:tcp, 2:udp, 3:both
    void create();
    void start();
    void stop();

    string getLocalIp();
    int getLocalPort();
    EventLoop::Ptr getLoop();
    Socket::Ptr getSocket() {return _socket;}
    sockaddr* getAddr() {return &_addr;}

    static void addClient(const string& key, const RtpClient::Ptr& client);
    static RtpClient::Ptr getClient(const string& key);

private:
    void onConnected();
    virtual void onRead(const StreamBuffer::Ptr& buffer);
    virtual void doPush();
    virtual void doPull();
    void send(const StreamBuffer::Ptr& buffer);

private:
    // bool _firstWrite = true;
    bool _sendFlag = true;
    string _peerIp;
    int _peerPort;
    int _sockType;
    int _ssrc;
    string _streamName;
    string _appName;

    Socket::Ptr _socket;
    sockaddr _addr;

    static mutex _mtx;
    // string : ip_port_socktype
    static unordered_map<string, RtpClient::Ptr> _mapClient;
};

#endif //RtpClient_h