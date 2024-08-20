#ifndef RtspPsEncodeTrack_H
#define RtspPsEncodeTrack_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Rtp/Decoder/RtpDecoder.h"
#include "Rtp/Encoder/RtpEncoder.h"
#include "RtspSdpParser.h"
#include "Common/Track.h"
#include "Rtp/RtpPacket.h"
#include "Common/StampAdjust.h"
#include "RtspTrack.h"
#include "Mpeg/PsMuxer.h"

using namespace std;

class RtspPsEncodeTrack : public RtspTrack, public enable_shared_from_this<RtspPsEncodeTrack>
{
public:
    using Ptr = shared_ptr<RtspPsEncodeTrack>;
    RtspPsEncodeTrack(int trackIndex);
    virtual ~RtspPsEncodeTrack()  {}

public:
    void onRtpPacket(const RtpPacket::Ptr& rtp, bool start) override;
    void onPsFrame(const FrameBuffer::Ptr& frame);
    void onFrame(const FrameBuffer::Ptr& frame) override;
    void startEncode() override;

    void setOnRtpPacket(const function<void(const RtpPacket::Ptr& rtp, bool start)>& cb) override {_onRtpPacket = cb;}

    int getTrackIndex()  override {return _index;}
    int getTrackType() {return _type;}
    shared_ptr<TrackInfo> getTrackInfo() override { return _trackInfo;}
    void addTrackInfo(const shared_ptr<TrackInfo>& track) {_mapTrackInfo[track->index_] = track;}

    bool hasSetup() {return _setup;}
    void setup(bool flag) {_setup = flag;}
    void setInterleavedRtp(int interleavedRtp) {_interleavedRtp = interleavedRtp;}
    int getInterleavedRtp() {return _interleavedRtp;}

private:
    bool _setup = false;
    int _index;
    int _type;
    int _interleavedRtp;
    RtpEncoder::Ptr _encoder;
    PsMuxer::Ptr _muxer;
    shared_ptr<SdpMedia> _media;
    shared_ptr<TrackInfo> _trackInfo;
    unordered_map<int, shared_ptr<TrackInfo>> _mapTrackInfo;
    function<void(const RtpPacket::Ptr& rtp, bool start)> _onRtpPacket;
};


#endif //RtspTrack_H
