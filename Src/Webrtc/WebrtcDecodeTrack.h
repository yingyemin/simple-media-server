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

// using namespace std;

class WebrtcDecodeTrack : public std::enable_shared_from_this<WebrtcDecodeTrack>
{
public:
    using Ptr = std::shared_ptr<WebrtcDecodeTrack>;
    WebrtcDecodeTrack(int trackIndex, int type, const std::shared_ptr<WebrtcPtInfo>& ptInfo);
    virtual ~WebrtcDecodeTrack()  {}

public:
    void onRtpPacket(const RtpPacket::Ptr& rtp);
    void startDecode();
    void stopDecode();

    void setOnRtpPacket(const std::function<void(const RtpPacket::Ptr& rtp)>& cb) {_onRtpPacket = cb;}
    void setOnFrame(const std::function<void(const FrameBuffer::Ptr& frame)>& cb) {_onFrame = cb;}
    void onFrame(const FrameBuffer::Ptr& frame);
    
    void setOnReady(const std::function<void()>& cb) {_onReady = cb;}

    int getTrackIndex()  {return _index;}
    int getTrackType() {return _type;}
    std::shared_ptr<TrackInfo> getTrackInfo() { return _trackInfo;}

    void decodeRtp(const RtpPacket::Ptr& rtp);

private:
    bool _enableDecode = false;
    bool _ready = false;
    int _index;
    int _type;
    RtpDecoder::Ptr _decoder;
    std::shared_ptr<WebrtcPtInfo> _ptInfo;
    std::shared_ptr<StampAdjust> _stampAdjust;
    std::shared_ptr<TrackInfo> _trackInfo;
    std::function<void(const FrameBuffer::Ptr& frame)> _onFrame;
    std::function<void()> _onReady;
    std::function<void(const RtpPacket::Ptr& rtp)> _onRtpPacket;
};


#endif //WebrtcDecodeTrack_H
