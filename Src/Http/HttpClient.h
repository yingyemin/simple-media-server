#ifndef HttpClient_h
#define HttpClient_h

#include "HttpParser.h"
#include "Common/UrlParser.h"
#include "Net/TcpClient.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

using namespace std;

class HttpClient : public TcpClient
{
public:
    HttpClient(const EventLoop::Ptr& loop);
    HttpClient(const EventLoop::Ptr& loop, bool enableTls);
    ~HttpClient();

public:
    int start(const string& localIp, int localPort, const string& peerIp, int peerPort, int timeout);
    int sendHeader(const string& localIp, int localPort, const string& url, int timeout);
    void setWebsocket() {_isWebsocket = true;}

    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError(const string& err) override;
    void close() override;
    void onConnect() override;

    void addHeader(const string& key, const string& header);
    void setContent(const string& content);
    void setMethod(const string& method);

    int sendHeader(const string& url, int timeout);
    void sendContent(const char* data, int len);
    void send(const string& msg);

    virtual void onHttpRequest();
    virtual void onRecvContent(const char *data, uint64_t len);

public:
    string _websocketKey;
    HttpParser _parser;
    HttpParser _request;

private:
    bool _isWebsocket = false;
    string _body;
    UrlParser _urlParser;
    EventLoop::Ptr _loop;
};

#endif //HttpClient_h