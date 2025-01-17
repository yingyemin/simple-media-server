#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpEncodeVP9.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

RtpEncodeVP9::RtpEncodeVP9(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{}

void RtpEncodeVP9::encode(const FrameBuffer::Ptr& frame)
{
    auto payload = (char *)frame->data() + frame->startSize();
    auto pts = frame->pts();
    auto size = frame->size() - frame->startSize();
    bool start = true;
    while (size > 0) {
        if (size <= _maxRtpSize) {
            makeRtp(payload, size, start, true, frame->startFrame(), pts);
            break;
        }
        makeRtp(payload, _maxRtpSize, start, false, frame->startFrame(), pts);
        payload += _maxRtpSize;
        size -= _maxRtpSize;
        start = false;
    }
    return ;
}

void RtpEncodeVP9::makeRtp(const char *data, size_t len, bool start, bool mark, bool gopStart, uint64_t pts)
{
    // rtp header 12
    // VP9 rtp header 1
    RtpPacket::Ptr rtp = RtpPacket::create(_trackInfo, len + 12 + 1, pts, _ssrc, _lastSeq++, mark);
    auto payload = rtp->getPayload();
    if (start) {
        payload[0] = 0x08;/*start of a layer frame*/
    } else {
        payload[0] = 0x00;
    }
    if (mark) {
        payload[0] |= 0x04; //End of a layer frame
    }
    memcpy(payload + 1, data, len);

    onRtpPacket(rtp, gopStart);
}

void RtpEncodeVP9::setOnRtpPacket(const function<void(const RtpPacket::Ptr& packet, bool start)>& cb)
{
    _onRtpPacket = cb;
}

void RtpEncodeVP9::onRtpPacket(const RtpPacket::Ptr& rtp, bool start)
{
    if (_onRtpPacket) {
        _onRtpPacket(rtp, start);
    }
}