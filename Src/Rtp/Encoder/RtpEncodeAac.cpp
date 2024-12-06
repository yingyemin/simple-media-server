#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpEncodeAac.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

RtpEncodeAac::RtpEncodeAac(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{}

void RtpEncodeAac::encode(const FrameBuffer::Ptr& frame)
{
    auto payload = (char *)frame->data() + frame->startSize();
    auto pts = frame->pts();
    auto size = frame->size() - frame->startSize();
    auto remain_size = size;
    auto aacMaxRtpSize = _maxRtpSize - 4;

    bool start = false;
    bool first = true;
    while (remain_size > 0) {
        if (first) {
            first = false;
            start = frame->startFrame();
        }
        bool mark = false;
        if (remain_size <= aacMaxRtpSize) {
            makeRtp(payload, remain_size, size, true, pts, start);
            break;
        }
        makeRtp(payload, aacMaxRtpSize, size, false, pts, start);
        payload += aacMaxRtpSize;
        remain_size -= aacMaxRtpSize;
    }
    return ;
}

void RtpEncodeAac::makeRtp(const char *data, size_t len, size_t total_len, bool mark, uint64_t pts, bool start)
{
    RtpPacket::Ptr rtp = RtpPacket::create(_trackInfo, len + 4 + 12, pts, _ssrc, _lastSeq++, mark);
    auto payload = rtp->getPayload();
    payload[0] = 0;
    payload[1] = 16;
    payload[2] = ((total_len) >> 5) & 0xFF;
    payload[3] = ((total_len & 0x1F) << 3) & 0xFF;
    memcpy(payload + 4, data, len);

    onRtpPacket(rtp, start);
}

void RtpEncodeAac::setOnRtpPacket(const function<void(const RtpPacket::Ptr& packet, bool start)>& cb)
{
    _onRtpPacket = cb;
}

void RtpEncodeAac::onRtpPacket(const RtpPacket::Ptr& rtp, bool start)
{
    if (_onRtpPacket) {
        _onRtpPacket(rtp, start);
    }
}