#ifndef JT1078MediaSource_H
#define JT1078MediaSource_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>

#include "Common/MediaSource.h"
#include "JT1078DecodeTrack.h"
// #include "GB28181EncodeTrack.h"
#include "Common/DataQue.h"

using namespace std;

class JT1078MediaSource : public MediaSource
{
public:
    using Ptr = shared_ptr<JT1078MediaSource>;
    using Wptr = weak_ptr<JT1078MediaSource>;
    using RingDataType = shared_ptr<list<JT1078RtpPacket::Ptr> >;
    using RingType = DataQue<RingDataType>;

    JT1078MediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr, bool muxer = false);
    ~JT1078MediaSource();

public:
    void addTrack(const JT1078DecodeTrack::Ptr& track);
    void addDecodeTrack(const shared_ptr<TrackInfo>& track);
    void addTrack(const shared_ptr<TrackInfo>& track) override;
    void addSink(const MediaSource::Ptr &src) override;
    void delSink(const MediaSource::Ptr &src) override;
    // void onFrame(const FrameBuffer::Ptr& frame) override;
    void onReady() override;
    unordered_map<int/*index*/, JT1078DecodeTrack::Ptr> getDecodeTrack()
    {
        return _mapJT1078DecodeTrack;
    }
    // unordered_map<int/*index*/, GB28181EncodeTrack::Ptr> getEncodeTrack()
    // {
    //     return _mapGB28181EncodeTrack;
    // }

    RingType::Ptr getRing() {return _ring;}

private:
    bool _muxer;
    bool _probeFinish = false;
    int _ringSize = 512;

    mutex _mtxSdp;
    string _sdp;

    RingType::Ptr _ring;
    RingDataType _cache;

    mutex _mtxTrack;
    unordered_map<int/*index*/, JT1078DecodeTrack::Ptr> _mapJT1078DecodeTrack;
    // unordered_map<int/*index*/, GB28181EncodeTrack::Ptr> _mapGB28181EncodeTrack;
};


#endif //JT1078MediaSource_H
