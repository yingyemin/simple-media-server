#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpEncodeCommon.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

RtpEncodeCommon::RtpEncodeCommon(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{}

void RtpEncodeCommon::encode(const FrameBuffer::Ptr& frame)
{
    auto payload = (char *)frame->data() + frame->startSize();
    auto pts = frame->pts();
    auto size = frame->size() - frame->startSize();
    while (size > 0) {
        if (size <= _maxRtpSize) {
            makeRtp(payload, size, true, pts);
            break;
        }
        makeRtp(payload, _maxRtpSize, false, pts);
        payload += _maxRtpSize;
        size -= _maxRtpSize;
    }
    return ;
}

void RtpEncodeCommon::makeRtp(const char *data, size_t len, bool mark, uint64_t pts)
{
    RtpPacket::Ptr rtp = RtpPacket::create(_trackInfo, len + 12, pts, _ssrc, _lastSeq++, mark);
    auto payload = rtp->getPayload();
    memcpy(payload, data, len);

    if (_trackInfo->trackType_ == "video") {
        onRtpPacket(rtp, true);
    } else {
        onRtpPacket(rtp, false);
    }
}

void RtpEncodeCommon::setOnRtpPacket(const function<void(const RtpPacket::Ptr& packet, bool start)>& cb)
{
    _onRtpPacket = cb;
}

void RtpEncodeCommon::onRtpPacket(const RtpPacket::Ptr& rtp, bool start)
{
    if (_onRtpPacket) {
        _onRtpPacket(rtp, start);
    }
}