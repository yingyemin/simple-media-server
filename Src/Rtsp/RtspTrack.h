#ifndef RtspTrack_H
#define RtspTrack_H

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

using namespace std;

class RtspTrack
{
public:
    using Ptr = shared_ptr<RtspTrack>;

    virtual void startDecode() {}
    virtual void stopDecode() {}
    virtual void startEncode() {}

    virtual bool hasSetup() {return false;}
    virtual void setInterleavedRtp(int interleavedRtp) {}
    virtual int getInterleavedRtp() {return 0;}

    virtual void setOnRtpPacket(const function<void(const RtpPacket::Ptr& rtp, bool start)>& cb) {}
    virtual void onRtpPacket(const RtpPacket::Ptr& rtp, bool start) {}

    virtual shared_ptr<TrackInfo> getTrackInfo() {return nullptr;}
    virtual void decodeRtp(const RtpPacket::Ptr& rtp) {};
    
    virtual void setOnFrame(const function<void(const FrameBuffer::Ptr& frame)>& cb) {}
    virtual void onFrame(const FrameBuffer::Ptr& frame) {}
    // virtual void setRing(const DataQue<shared_ptr<list<RtpPacket::Ptr>>>& ring) {}

    virtual int getTrackIndex() {return -1;}

    virtual uint16_t getSeq() {return _seq;}
    virtual uint32_t getSsrc() {return _ssrc;}
    virtual uint32_t getTimestamp() {return _timestap;}
    virtual void setSsrc(uint16_t ssrc) {_ssrc = ssrc;}

protected:
    uint16_t _seq = 0;
    uint32_t _ssrc = 0;
    uint32_t _timestap = 0;
};

class RtspDecodeTrack : public RtspTrack, public enable_shared_from_this<RtspDecodeTrack>
{
public:
    using Ptr = shared_ptr<RtspDecodeTrack>;
    RtspDecodeTrack(int trackIndex, const shared_ptr<SdpMedia>& media);
    virtual ~RtspDecodeTrack()  {}

public:
    void onRtpPacket(const RtpPacket::Ptr& rtp, bool start) override;
    void startDecode() override;
    void stopDecode() override;

    void setOnRtpPacket(const function<void(const RtpPacket::Ptr& rtp, bool start)>& cb) override {_onRtpPacket = cb;}
    void setOnFrame(const function<void(const FrameBuffer::Ptr& frame)>& cb) override {_onFrame = cb;}
    void onFrame(const FrameBuffer::Ptr& frame) override;

    int getTrackIndex()  override {return _index;}
    int getTrackType() {return _type;}
    shared_ptr<TrackInfo> getTrackInfo() override { return _trackInfo;}

    bool hasSetup() {return _setup;}
    void setup(bool flag) {_setup = flag;}
    void setInterleavedRtp(int interleavedRtp) {_interleavedRtp = interleavedRtp;}
    int getInterleavedRtp() {return _interleavedRtp;}

    void decodeRtp(const RtpPacket::Ptr& rtp);

private:
    bool _setup = false;
    int _index;
    int _type;
    int _interleavedRtp;
    RtpDecoder::Ptr _decoder;
    shared_ptr<SdpMedia> _media;
    shared_ptr<StampAdjust> _stampAdjust;
    shared_ptr<TrackInfo> _trackInfo;
    function<void(const FrameBuffer::Ptr& frame)> _onFrame;
    function<void(const RtpPacket::Ptr& rtp, bool start)> _onRtpPacket;
};

class RtspEncodeTrack : public RtspTrack, public enable_shared_from_this<RtspEncodeTrack>
{
public:
    using Ptr = shared_ptr<RtspEncodeTrack>;
    RtspEncodeTrack(int trackIndex, const shared_ptr<TrackInfo>& trackInfo);
    virtual ~RtspEncodeTrack()  {}

public:
    void onRtpPacket(const RtpPacket::Ptr& rtp, bool start) override;
    void onFrame(const FrameBuffer::Ptr& frame) override;
    void startEncode() override;

    void setOnRtpPacket(const function<void(const RtpPacket::Ptr& rtp, bool start)>& cb) override {_onRtpPacket = cb;}

    int getTrackIndex()  override {return _index;}
    int getTrackType() {return _type;}
    shared_ptr<TrackInfo> getTrackInfo() override { return _trackInfo;}

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
    shared_ptr<SdpMedia> _media;
    shared_ptr<TrackInfo> _trackInfo;
    function<void(const RtpPacket::Ptr& rtp, bool start)> _onRtpPacket;
};


#endif //RtspTrack_H
