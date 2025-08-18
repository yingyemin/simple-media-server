#ifndef RtmpEncodeTrack_H
#define RtmpEncodeTrack_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Encode/RtmpEncode.h"
#include "Common/Track.h"

using namespace std;

class RtmpEncodeTrack : public enable_shared_from_this<RtmpEncodeTrack>
{
public:
    using Ptr = shared_ptr<RtmpEncodeTrack>;
    RtmpEncodeTrack(const shared_ptr<TrackInfo>& trackInfo);
    virtual ~RtmpEncodeTrack()  {}

public:
    void onRtmpPacket(const RtmpMessage::Ptr& pkt, bool start);
    void onFrame(const FrameBuffer::Ptr& frame);
    void startEncode();
    string getConfig();

    void setOnRtmpPacket(const function<void(const RtmpMessage::Ptr& rtp, bool start)>& cb) {_onRtmpPacket = cb;}

    int getTrackIndex()  {return _index;}
    int getTrackType() {return _type;}
    shared_ptr<TrackInfo> getTrackInfo() { return _trackInfo;}
    void setEnhanced(bool enhanced) {_enhanced = enhanced;}
    void setFastPts(bool enabled) {_enableFastPts = enabled;}

private:
    bool _enhanced = false;
    bool _enableFastPts = false;
    uint32_t _timestap;
    int _index;
    int _type;
    RtmpEncode::Ptr _encoder;
    shared_ptr<TrackInfo> _trackInfo;
    function<void(const RtmpMessage::Ptr& rtp, bool start)> _onRtmpPacket;
};


#endif //GB28181EncodeTrack_H
