#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtmpEncodeTrack.h"
#include "Logger.h"

using namespace std;

RtmpEncodeTrack::RtmpEncodeTrack(const shared_ptr<TrackInfo>& trackInfo)
    :_index(trackInfo->index_)
    ,_type(trackInfo->trackType_ == "video" ? VideoTrackType : AudioTrackType)
    ,_trackInfo(trackInfo)
{
    _trackInfo->ssrc_ = _trackInfo->index_;
}

void RtmpEncodeTrack::startEncode()
{
    weak_ptr<RtmpEncodeTrack> wSelf = dynamic_pointer_cast<RtmpEncodeTrack>(shared_from_this());
    if (!_encoder) {
        _encoder = RtmpEncode::create(_trackInfo);
        _encoder->setOnRtmpPacket([wSelf](const RtmpMessage::Ptr& pkt, bool start){
            auto self = wSelf.lock();
            if (self) {
                self->onRtmpPacket(pkt, start);
            }
        });
    }
}

void RtmpEncodeTrack::onFrame(const FrameBuffer::Ptr& frame)
{
    if (_encoder) {
        // logInfo << "encode a frame";
        _encoder->encode(frame);
    }
}

void RtmpEncodeTrack::onRtmpPacket(const RtmpMessage::Ptr& pkt, bool start)
{
    if (_onRtmpPacket) {
        _onRtmpPacket(pkt, start);
    }
    // logInfo << "encode a rtp packet";
    _timestap = pkt->abs_timestamp;
}
