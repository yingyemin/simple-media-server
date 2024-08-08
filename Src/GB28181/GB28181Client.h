#ifndef GB28181Client_h
#define GB28181Client_h

#include "Net/Socket.h"
#include "Net/TcpClient.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

using namespace std;

class GB28181Client : public enable_shared_from_this<GB28181Client> {
public:
    using Ptr = shared_ptr<GB28181Client>;
    using Wptr = weak_ptr<GB28181Client>;

    GB28181Client(const string& ip, int port, const string& app, 
                    const string& stream, int ssrc, int sockType, bool sendFlag);
    ~GB28181Client();

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

    static void addClient(const string& key, const GB28181Client::Ptr& client);
    static GB28181Client::Ptr getClient(const string& key);

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
    static unordered_map<string, GB28181Client::Ptr> _mapClient;
};

#endif //GB28181Client_h