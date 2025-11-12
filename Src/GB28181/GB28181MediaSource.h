#ifndef GB28181MediaSource_H
#define GB28181MediaSource_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>

#include "Common/MediaSource.h"
#include "GB28181DecodeTrack.h"
#include "GB28181EncodeTrack.h"
#include "Common/DataQue.h"

// using namespace std;

class GB28181MediaSource : public MediaSource
{
public:
    using Ptr = std::shared_ptr<GB28181MediaSource>;
    using Wptr = std::weak_ptr<GB28181MediaSource>;
    using RingDataType = std::shared_ptr<std::list<RtpPacket::Ptr> >;
    using RingType = DataQue<RingDataType>;

    GB28181MediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr, bool muxer = false);
    ~GB28181MediaSource();

public:
    void addTrack(const GB28181DecodeTrack::Ptr& track);
    void addDecodeTrack(const std::shared_ptr<TrackInfo>& track);
    void addTrack(const std::shared_ptr<TrackInfo>& track) override;
    void addSink(const MediaSource::Ptr &src) override;
    void delSink(const MediaSource::Ptr &src) override;
    void onFrame(const FrameBuffer::Ptr& frame) override;
    void onReady() override;
    int playerCount() override;
    void getClientList(const std::function<void(const std::list<ClientInfo>& info)>& func) override;
    uint64_t getBytes() override { return _ring ? _ring->getBytes() : 0;}
    std::unordered_map<int/*index*/, GB28181DecodeTrack::Ptr> getDecodeTrack()
    {
        return _mapGB28181DecodeTrack;
    }
    GB28181EncodeTrack::Ptr getEncodeTrack()
    {
        return _gB28181EncodeTrack;
    }

    void setSsrc(uint32_t ssrc) {_ssrc = ssrc;}

    RingType::Ptr getRing() {return _ring;}

private:
    bool _muxer;
    bool _probeFinish = false;
    int _ringSize = 512;
    uint32_t _ssrc = 0;

    std::mutex _mtxSdp;
    std::string _sdp;

    RingType::Ptr _ring;
    RingDataType _cache;

    GB28181EncodeTrack::Ptr _gB28181EncodeTrack;
    std::mutex _mtxTrack;
    std::unordered_map<int/*index*/, GB28181DecodeTrack::Ptr> _mapGB28181DecodeTrack;
};


#endif //GB28181MediaSource_H
