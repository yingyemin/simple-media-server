#ifndef GB28181EncodeTrack_H
#define GB28181EncodeTrack_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Rtp/Encoder/RtpEncoder.h"
#include "Common/Track.h"
#include "Rtp/RtpPacket.h"
#include "Mpeg/PsMuxer.h"

using namespace std;

class GB28181EncodeTrack : public enable_shared_from_this<GB28181EncodeTrack>
{
public:
    using Ptr = shared_ptr<GB28181EncodeTrack>;
    GB28181EncodeTrack(int trackIndex);
    virtual ~GB28181EncodeTrack()  {}

public:
    void onRtpPacket(const RtpPacket::Ptr& rtp);
    void onPsFrame(const FrameBuffer::Ptr& frame);
    void onFrame(const FrameBuffer::Ptr& frame);
    void startEncode();

    void setOnRtpPacket(const function<void(const RtpPacket::Ptr& rtp)>& cb) {_onRtpPacket = cb;}

    int getTrackIndex()  {return _index;}
    int getTrackType() {return _type;}
    void setSsrc(uint32_t ssrc) {_ssrc = ssrc;}
    shared_ptr<TrackInfo> getTrackInfo() { return _trackInfo;}
    void addTrackInfo(const shared_ptr<TrackInfo>& track) {_mapTrackInfo[track->index_] = track;}

private:
    int _index;
    int _type;
    uint32_t _ssrc = 0;
    RtpEncoder::Ptr _encoder;
    PsMuxer::Ptr _muxer;
    shared_ptr<TrackInfo> _trackInfo;
    unordered_map<int, shared_ptr<TrackInfo>> _mapTrackInfo;
    function<void(const RtpPacket::Ptr& rtp)> _onRtpPacket;
};


#endif //GB28181EncodeTrack_H
