#ifndef HttpTsClient_h
#define HttpTsClient_h

#include "Http/HttpClient.h"

#include <string>
#include <memory>
#include <functional>

using namespace std;

class HttpHlsTsClient : public HttpClient
{
public:
    using Ptr = shared_ptr<HttpHlsTsClient>;
    HttpHlsTsClient();
    ~HttpHlsTsClient();

public:
    // HttpClient override
    void onHttpRequest() override;
    void onRecvContent(const char *data, uint64_t len) override;
    void onConnect() override;
    void close() override;

public:
    void start(const string& localIp, int localPort, const string& url, int timeout);
    void onTsPacket(const char* tsPacket, int len);
    void onError(const string& err);
    void setOnTsPacket(const function<void(const char* tsPacket, int len)>& cb);

private:
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;

    function<void(const char* tsPacket, int len)> _onTsPacket;
    function<void()> _onClose;
};

#endif //HttpHlsClient_h