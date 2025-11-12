#ifndef RtmpMediaSource_H
#define RtmpMediaSource_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>

#include "Common/MediaSource.h"
#include "RtmpEncodeTrack.h"
#include "Common/DataQue.h"
#include "RtmpMessage.h"
#include "RtmpDecodeTrack.h"
#include "Amf.h"

// using namespace std;

class RtmpMediaSource : public MediaSource
{
public:
    using Ptr = std::shared_ptr<RtmpMediaSource>;
    using Wptr = std::weak_ptr<RtmpMediaSource>;
    using RingDataType = std::shared_ptr<list<RtmpMessage::Ptr> >;
    using RingType = DataQue<RingDataType>;

    RtmpMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr, bool muxer = false);
    virtual ~RtmpMediaSource();

public:
    void addTrack(const RtmpDecodeTrack::Ptr& track);
    void onReady() override;
    void addTrack(const std::shared_ptr<TrackInfo>& track) override;
    void addSink(const MediaSource::Ptr &src) override;
    void delSink(const MediaSource::Ptr &src) override;
    void onFrame(const FrameBuffer::Ptr& frame) override;
    // unordered_map<int/*index*/, RtspTrack::Ptr> getTrack() {return _mapRtspTrack;}

    void setMetadata(const AmfObjects& meta);
    AmfObjects getMetadata();
    void setAvcHeader(const StreamBuffer::Ptr& avcHeader, int avcHeaderSize){
        _avcHeaderSize = avcHeaderSize;
        _avcHeader = avcHeader;
    }
    void setAacHeader(const StreamBuffer::Ptr& aacHeader, int aacHeaderSize){
        _aacHeaderSize = aacHeaderSize;
        _aacHeader = aacHeader;
    }
    RingType::Ptr getRing() {return _ring;}

    int playerCount();
    void getClientList(const std::function<void(const std::list<ClientInfo>& info)>& func) override; 
    uint64_t getBytes() override { return _ring ? _ring->getBytes() : 0;}

    void setEnhanced(bool enhanced) {_enhanced = enhanced;}
    void setFastPts(bool enabled) {_enableFastPts = enabled;}

public:
    int _aacHeaderSize = 0;
    StreamBuffer::Ptr _aacHeader;

    int _avcHeaderSize = 0;
    StreamBuffer::Ptr _avcHeader;

private:
    bool _muxer;
    bool _start = false;
    bool _probeFinish = false;
    bool _enhanced = false;
    bool _enableFastPts = false;
    int _ringSize = 512;
    int64_t _lastPts = -1;

    std::mutex _mtxMeta;
    AmfObjects _metaData;

    RingType::Ptr _ring;
    RingDataType _cache;

    std::mutex _mtxTrack;
    std::unordered_map<int/*index*/, RtmpDecodeTrack::Ptr> _mapRtmpDecodeTrack;
    std::unordered_map<int/*index*/, RtmpEncodeTrack::Ptr> _mapRtmpEncodeTrack;
};


#endif //RtspMediaSource_H
