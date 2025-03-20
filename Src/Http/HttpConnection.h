#ifndef HttpConnection_h
#define HttpConnection_h

#include "TcpConnection.h"
#include "HttpParser.h"
#include "WebsocketContext.h"
#include "Common/UrlParser.h"
#include "HttpResponse.h"
#include "HttpFile.h"
#ifdef ENABLE_HLS
#include "Hls/HlsMediaSource.h"
#include "Hls/LLHlsMediaSource.h"
#endif
#ifdef ENABLE_MPEG
#include "Mpeg/TsMediaSource.h"
#include "Mpeg/PsMediaSource.h"
#endif
#ifdef ENABLE_MP4
#include "Mp4/Fmp4MediaSource.h"
#endif

#include <string>
#include <unordered_map>
#include <memory>

using namespace std;


class HttpConnection : public TcpConnection
{
public:
    using Ptr = shared_ptr<HttpConnection>;
    using Wptr = weak_ptr<HttpConnection>;

    HttpConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket, bool enbaleSsl);
    ~HttpConnection();

public:
    // 继承自tcpseesion
    virtual void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError() override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;

public:
    void setServerId(const string& key) {_serverId = key;}
    virtual void onWebsocketFrame(const char* data, int len) {}
    void apiRoute(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);

protected:
    void onHttpRequest();
    void writeHttpResponse(HttpResponse& rsp);
    void sendFile();
    void setFileRange(const string& rangeStr);

    virtual void handleGet();
    void handlePost();
    void handlePut();
    void handleDelete();
    void handleHead();
    void handleOptions();

    void handleFlvStream();
    void handleSmsHlsM3u8();
    void handleHlsM3u8();
#ifdef ENABLE_HLS
    void onPlayHls(const HlsMediaSource::Ptr &hlsSrc);
    void onPlayLLHls(const LLHlsMediaSource::Ptr &hlsSrc);
#endif
    void handleHlsTs();
    void handleLLHlsM3u8();
    void handleLLHlsTs();
    void handleTs();
    void handlePs();
#ifdef ENABLE_MPEG
    void onPlayTs(const TsMediaSource::Ptr &TsSrc);
    void onPlayPs(const PsMediaSource::Ptr &psSrc);
#endif
    void handleFmp4();
#ifdef ENABLE_MP4
    void onPlayFmp4(const Fmp4MediaSource::Ptr &fmp4Src);
#endif

    void onError(const string& msg);

protected:
    bool _isChunked = false;
    bool _isWebsocket = false;
    int _apiPort = 0;
    uint64_t _totalSendBytes = 0;
    uint64_t _intervalSendBytes = 0;
    float _lastBitrate = 0;
    string _rangeStr;
    string _mimeType;
    string _serverId;
    HttpParser _parser;
    UrlParser _urlParser;
    WebsocketContext _websocket;
    shared_ptr<HttpFile> _httpFile;

#ifdef ENABLE_MPEG
    TsMediaSource::RingType::DataQueReaderT::Ptr _playTsReader;
    PsMediaSource::RingType::DataQueReaderT::Ptr _playPsReader;
#endif

#ifdef ENABLE_MP4
    Fmp4MediaSource::RingType::DataQueReaderT::Ptr _playFmp4Reader;
#endif

    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    function<void()> _onClose;
    function<void(const char* data, int len)> _onHttpBody;
};

#endif /* HttpConnection_h */