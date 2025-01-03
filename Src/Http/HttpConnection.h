#ifndef HttpConnection_h
#define HttpConnection_h

#include "TcpConnection.h"
#include "HttpParser.h"
#include "WebsocketContext.h"
#include "Common/UrlParser.h"
#include "HttpResponse.h"
#include "HttpFile.h"
#include "Hls/HlsMediaSource.h"
#include "Hls/LLHlsMediaSource.h"
#include "Mpeg/TsMediaSource.h"
#include "Mpeg/PsMediaSource.h"
#include "Mp4/Fmp4MediaSource.h"

#include <string>
#include <unordered_map>
#include <memory>

using namespace std;


class HttpConnection : public TcpConnection {
public:
    using Ptr = shared_ptr<HttpConnection>;
    using Wptr = weak_ptr<HttpConnection>;

    HttpConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket, bool enbaleSsl);
    ~HttpConnection();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError() override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;

    void setServerId(const string& key) {_serverId = key;}

public:
    virtual void onWebsocketFrame(const char* data, int len) {}

private:
    void onHttpRequest();
    void writeHttpResponse(HttpResponse& rsp);
    void sendFile();
    void setFileRange(const string& rangeStr);

    void handleGet();
    void handlePost();
    void handlePut();
    void handleDelete();
    void handleHead();
    void handleOptions();

    void handleFlvStream();
    void handleSmsHlsM3u8();
    void handleHlsM3u8();
    void onPlayHls(const HlsMediaSource::Ptr &hlsSrc);
    void handleHlsTs();
    void handleLLHlsM3u8();
    void onPlayLLHls(const LLHlsMediaSource::Ptr &hlsSrc);
    void handleLLHlsTs();
    void handleTs();
    void onPlayTs(const TsMediaSource::Ptr &TsSrc);
    void handlePs();
    void onPlayPs(const PsMediaSource::Ptr &psSrc);
    void handleFmp4();
    void onPlayFmp4(const Fmp4MediaSource::Ptr &fmp4Src);

    void onError(const string& msg);

private:
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
    TsMediaSource::RingType::DataQueReaderT::Ptr _playTsReader;
    PsMediaSource::RingType::DataQueReaderT::Ptr _playPsReader;
    Fmp4MediaSource::RingType::DataQueReaderT::Ptr _playFmp4Reader;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    function<void()> _onClose;
    function<void(const char* data, int len)> _onHttpBody;
};

#endif /* HttpConnection_h */