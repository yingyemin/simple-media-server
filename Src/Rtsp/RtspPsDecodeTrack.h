#ifndef RtspPsDecodeTrack_H
#define RtspPsDecodeTrack_H

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
#include "Mpeg/PsDemuxer.h"

// using namespace std;

class RtspPsDecodeTrack : public RtspTrack, public std::enable_shared_from_this<RtspPsDecodeTrack>
{
public:
    using Ptr = std::shared_ptr<RtspPsDecodeTrack>;
    RtspPsDecodeTrack(int trackIndex, const std::shared_ptr<SdpMedia>& media);
    virtual ~RtspPsDecodeTrack()  {}

public:
    void onRtpPacket(const RtpPacket::Ptr& rtp, bool start) override;
    void startDecode() override;
    void stopDecode() override;

    void setOnRtpPacket(const std::function<void(const RtpPacket::Ptr& rtp, bool start)>& cb) override {_onRtpPacket = cb;}
    void setOnFrame(const std::function<void(const FrameBuffer::Ptr& frame)>& cb) override {_onFrame = cb;}
    void onFrame(const FrameBuffer::Ptr& frame) override;
    void setOnPsFrame(const std::function<void(const FrameBuffer::Ptr& frame)>& cb) {_onPsFrame = cb;}
    void onPsFrame(const FrameBuffer::Ptr frame);

    int getTrackIndex()  override {return _index;}
    int getTrackType() {return _type;}
    std::shared_ptr<TrackInfo> getTrackInfo() override { return _trackInfo;}

    bool hasSetup() {return _setup;}
    void setup(bool flag) {_setup = flag;}
    void setInterleavedRtp(int interleavedRtp) {_interleavedRtp = interleavedRtp;}
    int getInterleavedRtp() {return _interleavedRtp;}

    void decodeRtp(const RtpPacket::Ptr& rtp);

    void setOnTrackInfo(const std::function<void(const std::shared_ptr<TrackInfo>& trackInfo)>& cb);
    void setOnReady(const std::function<void()>& cb);

private:
    bool _startDemuxer = false;
    bool _startdecode = false;
    bool _setup = false;
    int _index;
    int _type;
    int _interleavedRtp;
    RtpDecoder::Ptr _decoder;
    std::shared_ptr<PsDemuxer> _demuxer;
    std::shared_ptr<SdpMedia> _media;
    std::shared_ptr<StampAdjust> _stampAdjust;
    std::shared_ptr<TrackInfo> _trackInfo;
    std::function<void(const FrameBuffer::Ptr& frame)> _onFrame;
    std::function<void(const RtpPacket::Ptr& rtp, bool start)> _onRtpPacket;
    std::function<void(const FrameBuffer::Ptr& frame)> _onPsFrame;
    std::function<void(const std::shared_ptr<TrackInfo>& trackInfo)> _onTrackInfo;
    std::function<void()> _onReady;
};

#endif //RtspPsDecodeTrack_H
