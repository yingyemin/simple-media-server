#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpEncodeMp3.h"
#include "Logger.h"
#include "Util/String.hpp"

using namespace std;

RtpEncodeMp3::RtpEncodeMp3(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{
    _trackInfo->samplerate_ = 90000;
}
// TODO rfc3119 打包格式


//MPEG Audio-specific header
// rfc2250
/*
 0               1               2               3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|             MBZ               |            Frag_offset        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
void RtpEncodeMp3::encode(const FrameBuffer::Ptr& frame)
{
    auto payload = (char *)frame->data() + frame->startSize();
    auto pts = frame->pts();
    auto size = frame->size() - frame->startSize();
    int maxRtpSize = _maxRtpSize;
    uint32_t offset = 0;

    if (size > INT32_MAX) {
        return ;
    }

    while (size > 0) {
        if (size <= maxRtpSize) {
            makeRtp(payload, size, true, pts, offset);
            break;
        }
        makeRtp(payload, maxRtpSize, false, pts, offset);
        payload += maxRtpSize;
        size -= maxRtpSize;
        offset += maxRtpSize;
    }
    return ;
}

void RtpEncodeMp3::makeRtp(const char *data, size_t len, bool mark, uint64_t pts, uint64_t offset)
{
    RtpPacket::Ptr rtp = RtpPacket::create(_trackInfo, len + 12 + 4, pts, _ssrc, _lastSeq++, mark);
    auto payload = rtp->getPayload();

    writeUint16BE((char*)payload, 0);
    writeUint16BE((char*)payload + 2, offset);
    memcpy(payload + 4, data, len);

    onRtpPacket(rtp, false);
}

void RtpEncodeMp3::setOnRtpPacket(const function<void(const RtpPacket::Ptr& packet, bool start)>& cb)
{
    _onRtpPacket = cb;
}

void RtpEncodeMp3::onRtpPacket(const RtpPacket::Ptr& rtp, bool start)
{
    if (_onRtpPacket) {
        _onRtpPacket(rtp, start);
    }
}