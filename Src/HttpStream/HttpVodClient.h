#ifndef HttpVodClient_h
#define HttpVodClient_h

#include "Common/MediaClient.h"
#include "HttpDownload.h"
#include "Http/HttpChunkedParser.h"
#include "Common/FrameMediaSource.h"

#include <string>
#include <memory>
#include <functional>
#include <deque>

// using namespace std;

class HttpVodClient :  public MediaClient, public std::enable_shared_from_this<HttpVodClient>
{
public:
    HttpVodClient(MediaClientType type, const std::string& appName, const std::string& streamName);
    virtual ~HttpVodClient();

public:
    // override MediaClient
    virtual bool start(const std::string& localIp, int localPort, const std::string& url, int timeout) override;
    void stop() override;
    void pause() override;
    void setOnClose(const std::function<void()>& cb) override;
    void getProtocolAndType(std::string& protocol, MediaClientType& type) override;

public:
    void onError(const std::string& err);
    void close();
    void download();
    virtual void onFrame(const FrameBuffer::Ptr& frame);
    void checkFrame();

    virtual void decode(const char* data, int len) {}

protected:
    bool _hasVideo = false;
    bool _hasAudio = false;
    bool _isDownload = false;
    bool _first = true;
    bool _complete = false;

    int _localPort = 0;
    int _timeout = 5;
    std::string _localIp;
    std::string _url;

    uint64_t _curPos = 0;
    uint64_t _readSize = 4 * 1024 * 1024;
    uint64_t _basePts = 0;

    TimeClock _clock;
    MediaClientType _type;
    UrlParser _localUrlParser;

    HttpDownload::Ptr _downloader;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    FrameMediaSource::Wptr _source;

    std::function<void()> _onClose;
    std::deque<FrameBuffer::Ptr> _frameList;
    std::unordered_map<int, TrackInfo::Ptr> _mapTrackInfo;
};

#endif //HttpFlvClient_h