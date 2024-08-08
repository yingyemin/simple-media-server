#ifndef HlsClientContext_h
#define HlsClientContext_h

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

using namespace std;

class HlsClientContext : public MediaClient, public enable_shared_from_this<HlsClientContext>
{
public:
    HlsClientContext(MediaClientType type, const string& appName, const string& streamName);
    ~HlsClientContext();

public:
    // override MediaClient
    void start(const string& localIp, int localPort, const string& url, int timeout) override;
    void stop() override;
    void pause() override;
    void setOnClose(const function<void()>& cb) override;

private:
    string getM3u8();
    void getM3u8Inner();
    void putTsList();
    void startPlay();
    void onPlay();
    void downloadTs();

private:
    MediaClientType _type;
    UrlParser _localUrlParser;

    string _originUrl;
    string _localIp;
    int _localPort;
    int _timeout;
    int _preTsIndex = -1;
    uint64_t _lastPts = 0;
    uint64_t _lastPlayStamp = 0;

    string _m3u8;
    
    TsDemuxer _tsDemuxer;
    TimeClock _playClock;
    TimeClock _aliveClock;
    shared_ptr<TimerTask> _timeTask;
    EventLoop::Ptr _loop;
    HttpHlsClient::Ptr _hlsClient;
    HttpHlsTsClient::Ptr _tsClient;
    FrameMediaSource::Wptr _source;

    map<int, TsInfo> _tsList;
    multimap<int, FrameBuffer::Ptr> _frameList;

    function<void()> _onClose;
};

#endif // HlsClientContext_h