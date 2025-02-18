#ifndef HttpPsClient_h
#define HttpPsClient_h

#include "Common/MediaClient.h"
#include "Mpeg/PsDemuxer.h"
#include "Http/HttpClient.h"
#include "Http/HttpChunkedParser.h"
#include "Mpeg/PsMediaSource.h"

#include <string>
#include <memory>
#include <functional>

using namespace std;

class HttpPsClient : public HttpClient, public MediaClient
{
public:
    HttpPsClient(MediaClientType type, const string& appName, const string& streamName);
    ~HttpPsClient();

public:
    // HttpClient override
    void onHttpRequest() override;
    void onRecvContent(const char *data, uint64_t len) override;
    void onConnect() override;
    void close() override;

public:
    // override MediaClient
    bool start(const string& localIp, int localPort, const string& url, int timeout) override;
    void stop() override;
    void pause() override;
    void setOnClose(const function<void()>& cb) override;

public:
    void onHttpResponce();
    void onError(const string& err);
    void setOnHttpResponce(const function<void(const HttpParser& parser)>& cb);

private:
    bool _hasVideo = false;
    bool _hasAudio = false;
    MediaClientType _type;
    UrlParser _localUrlParser;
    PsDemuxer _demuxer;
    
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    PsMediaSource::Wptr _source;
    HttpChunkedParser::Ptr _chunkedParser;

    function<void(const HttpParser& parser)> _onHttpResponce;
    function<void()> _onClose;
};

#endif //HttpFlvClient_h