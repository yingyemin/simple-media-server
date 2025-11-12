#ifndef HttpHlsClient_h
#define HttpHlsClient_h

#ifdef ENABLE_HLS

#include "Http/HttpClient.h"
#include "Hls/HlsParser.h"
#include "Common/FrameMediaSource.h"

#include <string>
#include <memory>
#include <functional>

// using namespace std;

class HttpHlsClient : public HttpClient
{
public:
    using Ptr = std::shared_ptr<HttpHlsClient>;
    HttpHlsClient();
    ~HttpHlsClient();

public:
    // HttpClient override
    void onHttpRequest() override;
    void onRecvContent(const char *data, uint64_t len) override;
    void onConnect() override;
    void close() override;

public:
    bool start(const std::string& localIp, int localPort, const std::string& url, int timeout);
    void onHttpResponce();
    void onError(const std::string& err);
    void setOnHttpResponce(const std::function<void(const HttpParser& parser)>& cb);

public:
    std::string _m3u8;
    
    std::shared_ptr<HlsParser> _hlsParser;

private:
    UrlParser _localUrlParser;

    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    FrameMediaSource::Wptr _source;

    std::function<void(const HttpParser& parser)> _onHttpResponce;
    std::function<void()> _onClose;
};

#endif
#endif //HttpHlsClient_h