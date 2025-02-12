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
        if (!_encoder) {
            logWarn << "create encoder failed, codec: " << _trackInfo->codec_;
            return ;
        }
        _encoder->setOnRtmpPacket([wSelf](const RtmpMessage::Ptr& pkt, bool start){
            auto self = wSelf.lock();
            if (self) {
                self->onRtmpPacket(pkt, start);
            }
        });
        _encoder->setEnhanced(_enhanced);
        _encoder->setEnhanced(_enableFastPts);
        if (_trackInfo->codec_ == "vp8" || _trackInfo->codec_ == "vp9" || _trackInfo->codec_ == "h264" || _trackInfo->codec_ == "av1") {
            _encoder->setEnhanced(true);
        }
    }
}

void RtmpEncodeTrack::onFrame(const FrameBuffer::Ptr& frame)
{
    if (_encoder && frame) {
        // logInfo << "encode a frame";
        _encoder->encode(frame);
    }
}

void RtmpEncodeTrack::onRtmpPacket(const RtmpMessage::Ptr& pkt, bool start)
{
    if (_onRtmpPacket && pkt) {
        _onRtmpPacket(pkt, start);
    }
    // logInfo << "encode a rtp packet";
    _timestap = pkt->abs_timestamp;
}
