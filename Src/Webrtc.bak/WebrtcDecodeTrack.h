#ifndef WebrtcDecodeTrack_H
#define WebrtcDecodeTrack_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Rtp/Decoder/RtpDecoder.h"
#include "Common/Track.h"
#include "Rtp/RtpPacket.h"
#include "Common/StampAdjust.h"
#include "WebrtcSdpParser.h"

using namespace std;

class WebrtcDecodeTrack : public enable_shared_from_this<WebrtcDecodeTrack>
{
public:
    using Ptr = shared_ptr<WebrtcDecodeTrack>;
    WebrtcDecodeTrack(int trackIndex, int type, const shared_ptr<WebrtcPtInfo>& ptInfo);
    virtual ~WebrtcDecodeTrack()  {}

public:
    void onRtpPacket(const RtpPacket::Ptr& rtp);
    void startDecode();
    void stopDecode();

    void setOnRtpPacket(const function<void(const RtpPacket::Ptr& rtp)>& cb) {_onRtpPacket = cb;}
    void setOnFrame(const function<void(const FrameBuffer::Ptr& frame)>& cb) {_onFrame = cb;}
    void onFrame(const FrameBuffer::Ptr& frame);

    int getTrackIndex()  {return _index;}
    int getTrackType() {return _type;}
    shared_ptr<TrackInfo> getTrackInfo() { return _trackInfo;}

    void decodeRtp(const RtpPacket::Ptr& rtp);

private:
    int _index;
    int _type;
    RtpDecoder::Ptr _decoder;
    shared_ptr<WebrtcPtInfo> _ptInfo;
    shared_ptr<StampAdjust> _stampAdjust;
    shared_ptr<TrackInfo> _trackInfo;
    function<void(const FrameBuffer::Ptr& frame)> _onFrame;
    function<void(const RtpPacket::Ptr& rtp)> _onRtpPacket;
};


#endif //WebrtcDecodeTrack_H
