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

using namespace std;

class RtmpMediaSource : public MediaSource
{
public:
    using Ptr = shared_ptr<RtmpMediaSource>;
    using Wptr = weak_ptr<RtmpMediaSource>;
    using RingDataType = shared_ptr<list<RtmpMessage::Ptr> >;
    using RingType = DataQue<RingDataType>;

    RtmpMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr, bool muxer = false);
    virtual ~RtmpMediaSource();

public:
    void addTrack(const RtmpDecodeTrack::Ptr& track);
    void addTrack(const shared_ptr<TrackInfo>& track) override;
    void addSink(const MediaSource::Ptr &src) override;
    void delSink(const MediaSource::Ptr &src) override;
    void onFrame(const FrameBuffer::Ptr& frame) override;
    // unordered_map<int/*index*/, RtspTrack::Ptr> getTrack() {return _mapRtspTrack;}

    void setMetadata(const AmfObjects& meta);
    AmfObjects getMetadata();
    void setAvcHeader(shared_ptr<char> avcHeader, int avcHeaderSize){
        _avcHeaderSize = avcHeaderSize;
        _avcHeader = avcHeader;
    }
    void setAacHeader(shared_ptr<char> aacHeader, int aacHeaderSize){
        _aacHeaderSize = aacHeaderSize;
        _aacHeader = aacHeader;
    }
    RingType::Ptr getRing() {return _ring;}

    int playerCount();
    void getClientList(const function<void(const list<ClientInfo>& info)>& func) override;
    uint64_t getBytes() override { return _ring ? _ring->getBytes() : 0;}

public:
    int _aacHeaderSize = 0;
    shared_ptr<char> _aacHeader;

    int _avcHeaderSize = 0;
    shared_ptr<char> _avcHeader;

private:
    bool _muxer;
    bool _start = false;
    int _ringSize = 512;
    int64_t _lastPts = -1;

    mutex _mtxMeta;
    AmfObjects _metaData;

    RingType::Ptr _ring;
    RingDataType _cache;

    mutex _mtxTrack;
    unordered_map<int/*index*/, RtmpDecodeTrack::Ptr> _mapRtmpDecodeTrack;
    unordered_map<int/*index*/, RtmpEncodeTrack::Ptr> _mapRtmpEncodeTrack;
};


#endif //RtspMediaSource_H
