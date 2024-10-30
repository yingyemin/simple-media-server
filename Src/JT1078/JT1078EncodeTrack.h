#ifndef JT1078EncodeTrack_H
#define JT1078EncodeTrack_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "JT1078RtpPacket.h"
#include "Common/Track.h"
#include "Common/Frame.h"

using namespace std;

class JT1078EncodeTrack : public enable_shared_from_this<JT1078EncodeTrack>
{
public:
    using Ptr = shared_ptr<JT1078EncodeTrack>;
    JT1078EncodeTrack(const shared_ptr<TrackInfo>& trackInfo, const string& simCode, int channel);
    ~JT1078EncodeTrack()  {}

public:
    void onRtpPacket(const JT1078RtpPacket::Ptr& rtp);
    void onFrame(const FrameBuffer::Ptr& frame);
    void startEncode();
    void stopEncode();

    void setOnRtpPacket(const function<void(const JT1078RtpPacket::Ptr& rtp)>& cb) {_onRtpPacket = cb;}

    int getTrackIndex()  {return _trackInfo->index_;}
    int getTrackType() {return _type;}
    shared_ptr<TrackInfo> getTrackInfo() { return _trackInfo;}

private:
    void createSingleRtp(const FrameBuffer::Ptr& frame);
    void createMultiRtp(const FrameBuffer::Ptr& frame);

private:
    bool _startEncode = false;
    bool _firstPkt = true;
    int _type;
    int _channel = 0;
    uint16_t _seq = 0;
    uint64_t _lastFrameStamp = 0;
    uint64_t _lastKeyframeStamp = 0;
    string _simCode;
    shared_ptr<TrackInfo> _trackInfo;
    function<void(const JT1078RtpPacket::Ptr& rtp)> _onRtpPacket;
};


#endif //GB28181EncodeTrack_H
