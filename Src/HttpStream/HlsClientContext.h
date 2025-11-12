#ifndef HlsClientContext_h
#define HlsClientContext_h

#ifdef ENABLE_HLS

#include "Common/MediaClient.h"
#include "HttpStream/HttpHlsClient.h"
#include "HttpStream/HttpHlsTsClient.h"
#include "Common/FrameMediaSource.h"
#include "EventPoller/Timer.h"
#include "Util/TimeClock.h"
#include "Mpeg/TsDemuxer.h"

#include <string>
#include <memory>
#include <functional>

// using namespace std;

class HlsClientContext : public MediaClient, public std::enable_shared_from_this<HlsClientContext>
{
public:
    HlsClientContext(MediaClientType type, const std::string& appName, const std::string& streamName);
    ~HlsClientContext();

public:
    // override MediaClient
    bool start(const std::string& localIp, int localPort, const std::string& url, int timeout) override;
    void stop() override;
    void pause() override;
    void setOnClose(const std::function<void()>& cb) override;
    void getProtocolAndType(std::string& protocol, MediaClientType& type) override;

private:
    std::string getM3u8();
    void getM3u8Inner();
    void putTsList();
    void startPlay();
    void onPlay();
    void downloadTs();

private:
    MediaClientType _type;
    UrlParser _localUrlParser;

    std::string _originUrl;
    std::string _localIp;
    int _localPort;
    int _timeout;
    int _preTsIndex = -1;
    uint64_t _lastPts = 0;
    uint64_t _lastPlayStamp = 0;

    std::string _m3u8;

    TsDemuxer _tsDemuxer;
    TimeClock _playClock;
    TimeClock _aliveClock;
    std::shared_ptr<TimerTask> _timeTask;
    EventLoop::Ptr _loop;
    HttpHlsClient::Ptr _hlsClient;
    HttpHlsTsClient::Ptr _tsClient;
    FrameMediaSource::Wptr _source;

    std::map<int, TsInfo> _tsList;
    std::multimap<int, FrameBuffer::Ptr> _frameList;

    std::function<void()> _onClose;
};

#endif
#endif // HlsClientContext_h