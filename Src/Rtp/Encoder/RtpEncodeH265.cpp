#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpEncodeH265.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

class FuHeader {
public:
#if __BYTE_ORDER == __BIG_ENDIAN
    unsigned start_bit: 1;
    unsigned end_bit: 1;
    unsigned nal_type: 6;
#else
    unsigned nal_type: 6;
    unsigned end_bit: 1;
    unsigned start_bit: 1;
#endif
};

// 配置读取
static const int maxRtpSize = 1400;

RtpEncodeH265::RtpEncodeH265(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{}

void RtpEncodeH265::encode(const FrameBuffer::Ptr& frame)
{
    auto size = frame->size();
    if (size > maxRtpSize)  {
        encodeFuA(frame);
    } else {
        encodeSingle(frame);
    }
}

void RtpEncodeH265::encodeFuA(const FrameBuffer::Ptr& frame) {
    auto size = frame->size() - frame->startSize();
    auto pts = frame->pts();
    bool first = true;
    auto frameData = frame->data() + frame->startSize();
    auto fuIndicator = frameData[1];
    auto fuHeader = (uint8_t)(frameData[0] >> 1) & 0x3F;
    FuHeader *fu_flags = (FuHeader *) (&fuHeader);

    size -= 2;
    frameData += 2;

    while (size + 3 > maxRtpSize) {
        RtpPacket::Ptr rtp = RtpPacket::create(_trackInfo, maxRtpSize + 12, pts, _lastSeq++, false);
        if (first) { //start
            fu_flags->start_bit = 1;
            first = false;
        } else { //middle
            fu_flags->start_bit = 0;
        }
        
        auto payload = rtp->getPayload();
        payload[0] = 49 << 1;
        payload[1] = fuIndicator; // 1;
        payload[2] = fuHeader;
        memcpy(payload + 3, (uint8_t *) frameData, maxRtpSize - 3);
        size = size + 3 - maxRtpSize;
        frameData = frameData + maxRtpSize - 3;

        onRtpPacket(rtp, false);
    }

    if (size > 0) { //end
        RtpPacket::Ptr rtp = RtpPacket::create(_trackInfo, size + 3 + 12, pts, _lastSeq++, pts != _lastPts);
        fu_flags->end_bit = 1;
        fu_flags->start_bit = 0;
        auto payload = rtp->getPayload();
        payload[0] = 49 << 1;
        payload[1] = fuIndicator;
        payload[2] = fuHeader;
        memcpy(payload + 3, (uint8_t *) frameData, size);

        onRtpPacket(rtp, false);
    }
}

void RtpEncodeH265::encodeSingle(const FrameBuffer::Ptr& frame) {
    auto size = frame->size() - frame->startSize();
    auto frameData = frame->data() + frame->startSize();
    auto pts = frame->pts();

    RtpPacket::Ptr rtp = RtpPacket::create(_trackInfo, size + 12, pts, _lastSeq++, pts != _lastPts);
    auto payload = rtp->getPayload();
    memcpy(payload, (uint8_t *) frameData, size);

    onRtpPacket(rtp, frame->startFrame());
}

void RtpEncodeH265::setOnRtpPacket(const function<void(const RtpPacket::Ptr& packet, bool start)>& cb)
{
    _onRtpPacket = cb;
}

void RtpEncodeH265::onRtpPacket(const RtpPacket::Ptr& rtp, bool start)
{
    if (_onRtpPacket) {
        _onRtpPacket(rtp, start);
    }
}