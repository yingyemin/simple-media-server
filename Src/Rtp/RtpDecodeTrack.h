#ifndef RtpDecodeTrack_H
#define RtpDecodeTrack_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Rtp/Decoder/RtpDecoder.h"
#include "Common/Track.h"
#include "Rtp/RtpPacket.h"
#include "Mpeg/PsDemuxer.h"
#include "Mpeg/TsDemuxer.h"

// using namespace std;

class RtpDecodeTrack : public std::enable_shared_from_this<RtpDecodeTrack>
{
public:
    using Ptr = std::shared_ptr<RtpDecodeTrack>;
    RtpDecodeTrack(int trackIndex);
    RtpDecodeTrack(int trackIndex, const std::string& payloadType);
    virtual ~RtpDecodeTrack()  {}

public:
    void startDecode();
    void stopDecode();

    bool isStart() {return _startdecode;}

    void onRtpPacket(const RtpPacket::Ptr& rtp);
    void setOnRtpPacket(const std::function<void(const RtpPacket::Ptr& rtp)>& cb) {_onRtpPacket = cb;}
    void setOnFrame(const std::function<void(const FrameBuffer::Ptr& frame)>& cb) {_onFrame = cb;}
    void onFrame(const FrameBuffer::Ptr& frame);
    void setOnPsFrame(const std::function<void(const FrameBuffer::Ptr& frame)>& cb) {_onPsFrame = cb;}
    void onPsFrame(const FrameBuffer::Ptr frame);
    void onTsFrame(const FrameBuffer::Ptr frame);

    int getTrackIndex()  {return _index;}
    int getTrackType() {return _type;}
    std::shared_ptr<TrackInfo> getTrackInfo() { return _trackInfo;}

    bool hasSetup() {return _setup;}
    void setup(bool flag) {_setup = flag;}
    void setInterleavedRtp(int interleavedRtp) {_interleavedRtp = interleavedRtp;}
    int getInterleavedRtp() {return _interleavedRtp;}

    void decodeRtp(const RtpPacket::Ptr& rtp);

    void setOnTrackInfo(const std::function<void(const std::shared_ptr<TrackInfo>& trackInfo)>& cb);
    void setOnReady(const std::function<void()>& cb);

    void createVideoTrack(const std::string& videoCodec);
    void createAudioTrack(const std::string& audioCodec, int channel, int sampleBit, int sampleRate);

private:
    bool _startDemuxer = false;
    bool _startdecode = false;
    bool _setup = false;
    bool _firstVps = true;
    bool _firstSps = true;
    bool _firstPps = true;
    bool _hasReady = false;
    uint16_t _seq;
    uint32_t _ssrc;
    uint32_t _timestap;
    int _index;
    int _type;
    int _interleavedRtp;
    std::string _payloadType;
    std::shared_ptr<PsDemuxer> _psDemuxer;
    std::shared_ptr<TsDemuxer> _tsDemuxer;
    RtpDecoder::Ptr _decoder;
    std::shared_ptr<TrackInfo> _trackInfo;
    std::function<void(const RtpPacket::Ptr& rtp)> _onRtpPacket;
    std::function<void(const FrameBuffer::Ptr& frame)> _onFrame;
    std::function<void(const FrameBuffer::Ptr& frame)> _onPsFrame;
    std::function<void(const std::shared_ptr<TrackInfo>& trackInfo)> _onTrackInfo;
    std::function<void()> _onReady;
};


#endif //RtpDecodeTrack_H
