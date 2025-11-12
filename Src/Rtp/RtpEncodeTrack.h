#ifndef RtpEncodeTrack_H
#define RtpEncodeTrack_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Rtp/Encoder/RtpEncoder.h"
#include "Common/Track.h"
#include "Rtp/RtpPacket.h"
#include "Mpeg/PsMuxer.h"
#include "Mpeg/TsMuxer.h"

// using namespace std;

class RtpEncodeTrack : public std::enable_shared_from_this<RtpEncodeTrack>
{
public:
    using Ptr = std::shared_ptr<RtpEncodeTrack>;
    RtpEncodeTrack(int trackIndex, const std::string& payloadType);
    virtual ~RtpEncodeTrack()  {}

public:
    void onRtpPacket(const RtpPacket::Ptr& rtp);
    void onPsFrame(const FrameBuffer::Ptr& frame);
    void onTsFrame(const FrameBuffer::Ptr& frame);
    void onFrame(const FrameBuffer::Ptr& frame);
    void startEncode();

    void setOnRtpPacket(const std::function<void(const RtpPacket::Ptr& rtp)>& cb) {_onRtpPacket = cb;}

    int getTrackIndex()  {return _index;}
    int getTrackType() {return _type;}
    void setSsrc(uint32_t ssrc) {_ssrc = ssrc;}
    std::shared_ptr<TrackInfo> getTrackInfo() { return _trackInfo;}
    void addTrackInfo(const std::shared_ptr<TrackInfo>& track) {_mapTrackInfo[track->index_] = track;}

private:
    int _index;
    int _type;
    uint32_t _ssrc = 0;
    std::string _payloadType;
    RtpEncoder::Ptr _encoder;
    PsMuxer::Ptr _psMuxer;
    TsMuxer::Ptr _tsMuxer;
    std::shared_ptr<TrackInfo> _trackInfo;
    std::unordered_map<int, std::shared_ptr<TrackInfo>> _mapTrackInfo;
    std::function<void(const RtpPacket::Ptr& rtp)> _onRtpPacket;
};


#endif //RtpEncodeTrack_H
