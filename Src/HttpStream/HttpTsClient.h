#ifndef HttpTsClient_h
#define HttpTsClient_h

#ifdef ENABLE_MPEG

#include "Common/MediaClient.h"
#include "Mpeg/TsDemuxer.h"
#include "Http/HttpClient.h"
#include "Http/HttpChunkedParser.h"
#include "Mpeg/TsMediaSource.h"

#include <string>
#include <memory>
#include <functional>

// using namespace std;

class HttpTsClient : public HttpClient, public MediaClient
{
public:
    HttpTsClient(MediaClientType type, const std::string& appName, const std::string& streamName);
    ~HttpTsClient();

public:
    // HttpClient override
    void onHttpRequest() override;
    void onRecvContent(const char *data, uint64_t len) override;
    void onConnect() override;
    void close() override;

public:
    // override MediaClient
    bool start(const std::string& localIp, int localPort, const std::string& url, int timeout) override;
    void stop() override;
    void pause() override;
    void setOnClose(const std::function<void()>& cb) override;

public:
    void onHttpResponce();
    void onError(const std::string& err);
    void setOnHttpResponce(const std::function<void(const HttpParser& parser)>& cb);

private:
    bool _hasVideo = false;
    bool _hasAudio = false;
    MediaClientType _type;
    UrlParser _localUrlParser;
    TsDemuxer _demuxer;
    
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    TsMediaSource::Wptr _source;
    HttpChunkedParser::Ptr _chunkedParser;

    std::function<void(const HttpParser& parser)> _onHttpResponce;
    std::function<void()> _onClose;
};

#endif
#endif //HttpFlvClient_h