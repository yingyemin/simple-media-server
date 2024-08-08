#ifndef WebrtcEncodeTrack_H
#define WebrtcEncodeTrack_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Rtp/Encoder/RtpEncoder.h"
#include "Common/Track.h"
#include "Rtp/RtpPacket.h"
#include "Common/StampAdjust.h"

using namespace std;

class WebrtcEncodeTrack : public enable_shared_from_this<WebrtcEncodeTrack>
{
public:
    using Ptr = shared_ptr<WebrtcEncodeTrack>;
    WebrtcEncodeTrack(int trackIndex, const shared_ptr<TrackInfo>& trackInfo);
    virtual ~WebrtcEncodeTrack()  {}

public:
    void onRtpPacket(const RtpPacket::Ptr& rtp, bool start);
    void onFrame(const FrameBuffer::Ptr& frame);
    void startEncode();

    void setOnRtpPacket(const function<void(const RtpPacket::Ptr& rtp, bool start)>& cb) {_onRtpPacket = cb;}

    int getTrackIndex()  {return _index;}
    int getTrackType() {return _type;}
    shared_ptr<TrackInfo> getTrackInfo() { return _trackInfo;}

private:
    int _index;
    int _type;

    RtpEncoder::Ptr _encoder;
    shared_ptr<TrackInfo> _trackInfo;
    function<void(const RtpPacket::Ptr& rtp, bool start)> _onRtpPacket;
};


#endif //WebrtcEncodeTrack_H
