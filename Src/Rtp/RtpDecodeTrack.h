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

using namespace std;

class RtpDecodeTrack : public enable_shared_from_this<RtpDecodeTrack>
{
public:
    using Ptr = shared_ptr<RtpDecodeTrack>;
    RtpDecodeTrack(int trackIndex);
    RtpDecodeTrack(int trackIndex, const string& payloadType);
    virtual ~RtpDecodeTrack()  {}

public:
    void startDecode();
    void stopDecode();

    bool isStart() {return _startdecode;}

    void onRtpPacket(const RtpPacket::Ptr& rtp);
    void setOnRtpPacket(const function<void(const RtpPacket::Ptr& rtp)>& cb) {_onRtpPacket = cb;}
    void setOnFrame(const function<void(const FrameBuffer::Ptr& frame)>& cb) {_onFrame = cb;}
    void onFrame(const FrameBuffer::Ptr& frame);
    void setOnPsFrame(const function<void(const FrameBuffer::Ptr& frame)>& cb) {_onPsFrame = cb;}
    void onPsFrame(const FrameBuffer::Ptr frame);
    void onTsFrame(const FrameBuffer::Ptr frame);

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

    void createVideoTrack(const string& videoCodec);
    void createAudioTrack(const string& audioCodec, int channel, int sampleBit, int sampleRate);

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
    string _payloadType;
    shared_ptr<PsDemuxer> _psDemuxer;
    shared_ptr<TsDemuxer> _tsDemuxer;
    RtpDecoder::Ptr _decoder;
    shared_ptr<TrackInfo> _trackInfo;
    function<void(const RtpPacket::Ptr& rtp)> _onRtpPacket;
    function<void(const FrameBuffer::Ptr& frame)> _onFrame;
    function<void(const FrameBuffer::Ptr& frame)> _onPsFrame;
    function<void(const shared_ptr<TrackInfo>& trackInfo)> _onTrackInfo;
    function<void()> _onReady;
};


#endif //RtpDecodeTrack_H
