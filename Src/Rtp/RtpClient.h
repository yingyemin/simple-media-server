#ifndef RtpClient_h
#define RtpClient_h

#include "Net/Socket.h"
#include "Net/TcpClient.h"
#include "Common/MediaClient.h"
#include "Common/UrlParser.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

// using namespace std;

class RtpClient : public MediaClient, public std::enable_shared_from_this<RtpClient> {
public:
    using Ptr = std::shared_ptr<RtpClient>;
    using Wptr = std::weak_ptr<RtpClient>;

    RtpClient(MediaClientType type, const std::string& app, 
                    const std::string& stream, int ssrc, int sockType);
    ~RtpClient();

public:
    // override MediaClient
    bool start(const std::string& localIp, int localPort, const std::string& url, int timeout) override;
    void stop() override;
    void pause() override;
    void setOnClose(const std::function<void()>& cb) override;

public:
    // sockType: 1:tcp, 2:udp, 3:both
    void create(const std::string& localIp, int localPort, const std::string& url);

    std::string getLocalIp();
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
    std::string _streamName;
    std::string _appName;

    Socket::Ptr _socket;
    sockaddr _addr;
    std::function<void()> _onClose;
};

#endif //RtpClient_h