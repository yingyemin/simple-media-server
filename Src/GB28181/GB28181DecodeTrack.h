#ifndef GB28181DecodeTrack_H
#define GB28181DecodeTrack_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Rtp/Decoder/RtpDecoder.h"
#include "Common/Track.h"
#include "Rtp/RtpPacket.h"
#include "Mpeg/PsDemuxer.h"

using namespace std;

class GB28181DecodeTrack : public enable_shared_from_this<GB28181DecodeTrack>
{
public:
    using Ptr = shared_ptr<GB28181DecodeTrack>;
    GB28181DecodeTrack(int trackIndex);
    virtual ~GB28181DecodeTrack()  {}

public:
    void startDecode();
    void stopDecode();

    void onRtpPacket(const RtpPacket::Ptr& rtp);
    void setOnRtpPacket(const function<void(const RtpPacket::Ptr& rtp)>& cb) {_onRtpPacket = cb;}
    void setOnFrame(const function<void(const FrameBuffer::Ptr& frame)>& cb) {_onFrame = cb;}
    void onFrame(const FrameBuffer::Ptr& frame);
    void setOnPsFrame(const function<void(const FrameBuffer::Ptr& frame)>& cb) {_onPsFrame = cb;}
    void onPsFrame(const FrameBuffer::Ptr frame);

    int getTrackIndex()  {return _index;}
    int getTrackType() {return _type;}
    shared_ptr<TrackInfo> getTrackInfo() { return _trackInfo;}

    bool hasSetup() {return _setup;}
    void setup(bool flag) {_setup = flag;}
    void setInterleavedRtp(int interleavedRtp) {_interleavedRtp = interleavedRtp;}
    int getInterleavedRtp() {return _interleavedRtp;}

    void decodeRtp(const RtpPacket::Ptr& rtp);

    void setOnTrackInfo(const function<void(const shared_ptr<TrackInfo>& trackInfo)>& cb);
    void setOnReady(const function<void()>& cb);

private:
    bool _startDemuxer = false;
    bool _startdecode = false;
    bool _setup = false;
    uint16_t _seq;
    uint32_t _ssrc;
    uint32_t _timestap;
    int _index;
    int _type;
    int _interleavedRtp;
    shared_ptr<PsDemuxer> _demuxer;
    RtpDecoder::Ptr _decoder;
    shared_ptr<TrackInfo> _trackInfo;
    function<void(const RtpPacket::Ptr& rtp)> _onRtpPacket;
    function<void(const FrameBuffer::Ptr& frame)> _onFrame;
    function<void(const FrameBuffer::Ptr& frame)> _onPsFrame;
    function<void(const shared_ptr<TrackInfo>& trackInfo)> _onTrackInfo;
    function<void()> _onReady;
};


#endif //GB28181DecodeTrack_H
