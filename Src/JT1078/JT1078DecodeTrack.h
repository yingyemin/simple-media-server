#ifndef JT1078DecodeTrack_H
#define JT1078DecodeTrack_H

#include "Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"
#include "JT1078RtpPacket.h"
#include "PcmTranscode/AdpcmaTranscode.h"
#include "PcmTranscode/G711Transcode.h"

#include <unordered_map>

using namespace std;

class JT1078DecodeTrack
{
public:
    using Ptr = shared_ptr<JT1078DecodeTrack>;
    JT1078DecodeTrack(int trackIndex);

public:
    void onRtpPacket(const JT1078RtpPacket::Ptr& rtp);
    void startDecode();
    void stopDecode();
    
    void setOnReady(const function<void()>& cb) {_onReady = cb;}
    void setOnRtpPacket(const function<void(const JT1078RtpPacket::Ptr& rtp, bool isStartGop)>& cb) {_onRtpPacket = cb;}
    void setOnFrame(const function<void(const FrameBuffer::Ptr& frame)>& cb) {_onFrame = cb;}
    void onFrame(const FrameBuffer::Ptr& frame);

    int getTrackIndex()  {return _index;}
    int getTrackType() {return _type;}
    shared_ptr<TrackInfo> getTrackInfo() { return _trackInfo;}

    void decodeRtp(const JT1078RtpPacket::Ptr& rtp);

    void setOnTrackInfo(const function<void(const shared_ptr<TrackInfo>& trackInfo)>& cb);
    void onTrackInfo(const shared_ptr<TrackInfo>& trackInfo);
    void createFrame();
    bool isReady() {return _ready;}

private:
    bool _startDecode = false;
    bool _lossRtp = false;
    bool _ready = false;
    bool _setAacCfg = false;
    bool _createInfoFailed = false;
    int _index;
    int _type;
    uint16_t _lastSeq = 0;
    string _originAudioCodec;
    adpcm_state _state;
    G711Transcode _g711aEncode;
    FrameBuffer::Ptr _frame;
    shared_ptr<TrackInfo> _trackInfo;
    function<void()> _onReady;
    function<void(const FrameBuffer::Ptr& frame)> _onFrame;
    function<void(const JT1078RtpPacket::Ptr& rtp, bool isStartGop)> _onRtpPacket;
    function<void(const shared_ptr<TrackInfo>& trackInfo)> _onTrackInfo;
};



#endif //JT1078DecodeTrack_H
