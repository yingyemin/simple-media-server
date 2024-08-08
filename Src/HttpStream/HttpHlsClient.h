#ifndef HttpHlsClient_h
#define HttpHlsClient_h

#include "Http/HttpClient.h"
#include "Hls/HlsParser.h"
#include "Common/FrameMediaSource.h"

#include <string>
#include <memory>
#include <functional>

using namespace std;

class HttpHlsClient : public HttpClient
{
public:
    using Ptr = shared_ptr<HttpHlsClient>;
    HttpHlsClient();
    ~HttpHlsClient();

public:
    // HttpClient override
    void onHttpRequest() override;
    void onRecvContent(const char *data, uint64_t len) override;
    void onConnect() override;
    void close() override;

public:
    void start(const string& localIp, int localPort, const string& url, int timeout);
    void onHttpResponce();
    void onError(const string& err);
    void setOnHttpResponce(const function<void(const HttpParser& parser)>& cb);

public:
    string _m3u8;
    
    shared_ptr<HlsParser> _hlsParser;

private:
    UrlParser _localUrlParser;

    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    FrameMediaSource::Wptr _source;

    function<void(const HttpParser& parser)> _onHttpResponce;
    function<void()> _onClose;
};

#endif //HttpHlsClient_h