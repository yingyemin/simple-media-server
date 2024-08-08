#ifndef RtmpDecodeTrack_H
#define RtmpDecodeTrack_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Decode/RtmpDecode.h"
#include "Common/Track.h"
#include "RtmpMessage.h"
#include "Common/Frame.h"

using namespace std;

class RtmpDecodeTrack : public enable_shared_from_this<RtmpDecodeTrack>
{
public:
    using Ptr = shared_ptr<RtmpDecodeTrack>;
    RtmpDecodeTrack(int trackIndex);
    virtual ~RtmpDecodeTrack()  {}

public:
    void startDecode();
    void stopDecode();
    int createTrackInfo(int trackType, int codeId);
    void createDecoder();
    void setConfigFrame(const RtmpMessage::Ptr& pkt);

    void onRtmpPacket(const RtmpMessage::Ptr& pkt);
    void setOnRtmpPacket(const function<void(const RtmpMessage::Ptr& pkt)>& cb) {_onRtmpPacket = cb;}
    void setOnFrame(const function<void(const FrameBuffer::Ptr& frame)>& cb) {_onFrame = cb;}
    void onFrame(const FrameBuffer::Ptr& frame);

    int getTrackIndex()  {return _trackInfo->index_;}
    shared_ptr<TrackInfo> getTrackInfo() { return _trackInfo;}

    void decodeRtmp(const RtmpMessage::Ptr& pkt);

    void setOnTrackInfo(const function<void(const shared_ptr<TrackInfo>& trackInfo)>& cb);
    void setOnReady(const function<void()>& cb);

private:
    uint32_t _timestap;
    int _index;
    RtmpDecode::Ptr _decoder;
    shared_ptr<TrackInfo> _trackInfo;
    function<void(const RtmpMessage::Ptr& rtp)> _onRtmpPacket;
    function<void(const FrameBuffer::Ptr& frame)> _onFrame;
    function<void(const shared_ptr<TrackInfo>& trackInfo)> _onTrackInfo;
    function<void()> _onReady;
};


#endif //GB28181DecodeTrack_H
