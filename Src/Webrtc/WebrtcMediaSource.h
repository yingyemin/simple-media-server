#ifndef WebrtcMediaSource_H
#define WebrtcMediaSource_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <deque>

#include "Common/MediaSource.h"
#include "WebrtcEncodeTrack.h"
#include "WebrtcDecodeTrack.h"
#include "Common/DataQue.h"
#include "WebrtcRtpPacket.h"

using namespace std;

class WebrtcMediaSource : public MediaSource
{
public:
    using Ptr = shared_ptr<WebrtcMediaSource>;
    using Wptr = weak_ptr<WebrtcMediaSource>;
    using DataType = shared_ptr<deque<RtpPacket::Ptr> >;
    using QueType = DataQue<DataType>;

    WebrtcMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr, bool muxer = false);
    virtual ~WebrtcMediaSource();

public:
    void addTrack(const WebrtcDecodeTrack::Ptr& track);
    void addTrack(const shared_ptr<TrackInfo>& track) override;
    void addSink(const MediaSource::Ptr &src) override;
    void delSink(const MediaSource::Ptr &src) override;
    void onFrame(const FrameBuffer::Ptr& frame) override;
    void onReady() override;
    int playerCount() override;
    void getClientList(const function<void(const list<ClientInfo>& info)>& func) override;
    uint64_t getBytes() override { return _ring ? _ring->getBytes() : 0;}

    QueType::Ptr getRing() {return _ring;}
    void processG711(const FrameBuffer::Ptr& frame, const WebrtcEncodeTrack::Ptr& track);

private:
    bool _muxer;
    bool _probeFinish = false;
    bool _start = false;
    int _ringSize = 512;
    uint64_t _lastRtpStmp = 0;
    
    int _channels = 0;
    uint32_t _frameDur = 20; // ms
    FrameBuffer::Ptr _cacheFrame;
    // StringBuffer _cacheFrame;
    // uint64_t _inSize = 0;
    // uint64_t _outSize = 0;
    // uint64_t _inPts = 0;

    QueType::Ptr _ring;
    DataType _cache;

    mutex _mtxTrack;
    unordered_map<int/*index*/, WebrtcEncodeTrack::Ptr> _mapWebrtcEncodeTrack;
    unordered_map<int/*index*/, WebrtcDecodeTrack::Ptr> _mapWebrtcDecodeTrack;
};


#endif //WebrtcMediaSource_H
