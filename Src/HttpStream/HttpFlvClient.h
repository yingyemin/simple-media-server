#ifndef HttpFlvClient_h
#define HttpFlvClient_h

#ifdef ENABLE_RTMP

#include "Common/MediaClient.h"
#include "Flv/FlvDemuxer.h"
#include "Http/HttpClient.h"
#include "Http/HttpChunkedParser.h"
#include "Rtmp/RtmpMediaSource.h"

#include <string>
#include <memory>
#include <functional>

using namespace std;

class HttpFlvClient : public HttpClient, public MediaClient
{
public:
    HttpFlvClient(MediaClientType type, const string& appName, const string& streamName);
    ~HttpFlvClient();

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
    void getProtocolAndType(string& protocol, MediaClientType& type) override;

public:
    void onHttpResponce();
    void onError(const string& err);
    void setOnHttpResponce(const function<void(const HttpParser& parser)>& cb);
    void handleAudio(const char* data, int len);
    void handleVideo(const char* data, int len);

private:
    bool _hasVideo = false;
    bool _hasAudio = false;
    bool _validVideoTrack = true;
    bool _validAudioTrack = true;
    MediaClientType _type;
    UrlParser _localUrlParser;
    FlvDemuxer _demuxer;

    int _avcHeaderSize = 0;
    StreamBuffer::Ptr _avcHeader;
    int _aacHeaderSize = 0;
    StreamBuffer::Ptr _aacHeader;

    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    RtmpMediaSource::Wptr _source;
    RtmpDecodeTrack::Ptr _rtmpVideoDecodeTrack;
    RtmpDecodeTrack::Ptr _rtmpAudioDecodeTrack;
    HttpChunkedParser::Ptr _chunkedParser;

    function<void(const HttpParser& parser)> _onHttpResponce;
    function<void()> _onClose;
};

#endif
#endif //HttpFlvClient_h