#ifndef HttpConnection_h
#define HttpConnection_h

#include "TcpConnection.h"
#include "HttpParser.h"
#include "Common/UrlParser.h"
#include "HttpResponse.h"
#include "HttpFile.h"

#include <string>
#include <unordered_map>
#include <memory>

// using namespace std;

class HttpConnection : public TcpConnection
{
public:
    using Ptr = std::shared_ptr<HttpConnection>;
    using Wptr = std::weak_ptr<HttpConnection>;

    HttpConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket, bool enbaleSsl);
    ~HttpConnection();

public:
    // 继承自tcpseesion
    virtual void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError(const std::string& errMsg) override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;

public:
    void setServerId(const std::string& key) {_serverId = key;}
    virtual void onWebsocketFrame(const char* data, int len) {}
    void apiRoute(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

protected:
    void onHttpRequest();
    void writeHttpResponse(HttpResponse& rsp);
    void sendFile();
    void setFileRange(const std::string& rangeStr);

    void handleOptions();

    void onHttpError(const std::string& msg);

protected:
    bool _isChunked = false;
    bool _isWebsocket = false;
    int _apiPort = 0;
    uint64_t _totalSendBytes = 0;
    uint64_t _intervalSendBytes = 0;
    float _lastBitrate = 0;
    std::string _rangeStr;
    std::string _mimeType;
    std::string _serverId;
    HttpParser _parser;
    UrlParser _urlParser;
    // WebsocketContext _websocket;
    std::shared_ptr<HttpFile> _httpFile;

    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    std::function<void()> _onClose;
    std::function<void(const char* data, int len)> _onHttpBody;

};

#endif /* HttpConnection_h */