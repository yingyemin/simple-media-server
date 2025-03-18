#ifndef GB28181Client_h
#define GB28181Client_h

#include "Net/Socket.h"
#include "Net/TcpClient.h"
#include "Common/MediaClient.h"
#include "Common/UrlParser.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

using namespace std;

class GB28181Client : public MediaClient, public enable_shared_from_this<GB28181Client> {
public:
    using Ptr = shared_ptr<GB28181Client>;
    using Wptr = weak_ptr<GB28181Client>;

    GB28181Client(MediaClientType type, const string& app, 
                    const string& stream, int ssrc, int sockType);
    ~GB28181Client();

public:
    // override MediaClient
    bool start(const string& localIp, int localPort, const string& url, int timeout) override;
    void stop() override;
    void pause() override;
    void setOnClose(const function<void()>& cb) override;

public:
    // sockType: 1:tcp, 2:udp, 3:both
    void create(const string& localIp, int localPort, const string& url);

    string getLocalIp();
    int getLocalPort();
    EventLoop::Ptr getLoop();
    Socket::Ptr getSocket() {return _socket;}
    sockaddr* getAddr() {return &_addr;}

private:
    void onConnected();
    virtual void onRead(const StreamBuffer::Ptr& buffer);
    virtual void doPush();
    virtual void doPull();
    void send(const StreamBuffer::Ptr& buffer);

private:
    // bool _firstWrite = true;
    MediaClientType _type;
    UrlParser _localUrlParser;
    UrlParser _peerUrlParser;
    int _sockType;
    int _ssrc;
    string _streamName;
    string _appName;

    Socket::Ptr _socket;
    sockaddr _addr;
    function<void()> _onClose;
};

#endif //GB28181Client_h