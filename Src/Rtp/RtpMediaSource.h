#ifndef RtpMediaSource_H
#define RtpMediaSource_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>

#include "Common/MediaSource.h"
#include "RtpDecodeTrack.h"
#include "RtpEncodeTrack.h"
#include "Common/DataQue.h"

using namespace std;

class RtpMediaSource : public MediaSource
{
public:
    using Ptr = shared_ptr<RtpMediaSource>;
    using Wptr = weak_ptr<RtpMediaSource>;
    using RingDataType = shared_ptr<list<RtpPacket::Ptr> >;
    using RingType = DataQue<RingDataType>;

    RtpMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr, bool muxer = false);
    ~RtpMediaSource();

public:
    void addTrack(const RtpDecodeTrack::Ptr& track);
    void addDecodeTrack(const shared_ptr<TrackInfo>& track);
    void addTrack(const shared_ptr<TrackInfo>& track) override;
    void addSink(const MediaSource::Ptr &src) override;
    void delSink(const MediaSource::Ptr &src) override;
    void onFrame(const FrameBuffer::Ptr& frame) override;
    void onReady() override;
    int playerCount() override;
    void getClientList(const function<void(const list<ClientInfo>& info)>& func) override;
    uint64_t getBytes() override { return _ring ? _ring->getBytes() : 0;}
    unordered_map<int/*index*/, RtpDecodeTrack::Ptr> getDecodeTrack()
    {
        return _mapRtpDecodeTrack;
    }
    // RtpEncodeTrack::Ptr getEncodeTrack()
    // {
    //     return _RtpEncodeTrack;
    // }

    void setSsrc(uint32_t ssrc) {_ssrc = ssrc;}

    RingType::Ptr getRing() {return _ring;}
    void setPayloadType(const string& payloadType) {_payloadType = payloadType;}

private:
    bool _muxer;
    bool _probeFinish = false;
    int _ringSize = 512;
    uint32_t _ssrc = 0;

    mutex _mtxSdp;
    string _sdp;

    string _payloadType;

    RingType::Ptr _ring;
    RingDataType _cache;

    // RtpEncodeTrack::Ptr _rtpEncodeTrack;
    unordered_map<int/*index*/, RtpEncodeTrack::Ptr> _mapRtpEncodeTrack;
    mutex _mtxTrack;
    unordered_map<int/*index*/, RtpDecodeTrack::Ptr> _mapRtpDecodeTrack;
};


#endif //RtpMediaSource_H
