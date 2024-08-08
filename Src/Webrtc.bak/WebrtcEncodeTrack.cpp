#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "WebrtcEncodeTrack.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/Base64.h"

using namespace std;

WebrtcEncodeTrack::WebrtcEncodeTrack(int trackIndex, const shared_ptr<TrackInfo>& trackInfo)
    :_index(trackIndex)
    ,_type(trackInfo->trackType_ == "video" ? VideoTrackType : AudioTrackType)
    ,_trackInfo(trackInfo)
{
    _trackInfo->ssrc_ = _trackInfo->index_;
}

void WebrtcEncodeTrack::startEncode()
{
    logInfo << "WebrtcEncodeTrack::startEncode";
    weak_ptr<WebrtcEncodeTrack> wSelf = dynamic_pointer_cast<WebrtcEncodeTrack>(shared_from_this());
    if (!_encoder) {
        _encoder = RtpEncoder::create(_trackInfo);
        _encoder->setOnRtpPacket([wSelf](const RtpPacket::Ptr& rtp){
            auto self = wSelf.lock();
            self->onRtpPacket(rtp);
        });
    }
}

void WebrtcEncodeTrack::onFrame(const FrameBuffer::Ptr& frame)
{
    if (_encoder) {
        // logInfo << "encode a frame";
        _encoder->encode(frame);
    }
}

void WebrtcEncodeTrack::onRtpPacket(const RtpPacket::Ptr& rtp)
{
    if (_onRtpPacket) {
        _onRtpPacket(rtp);
    }
}
