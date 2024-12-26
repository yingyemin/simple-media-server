#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpEncodeH264.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

class FuHeader {
public:
#if __BYTE_ORDER == __BIG_ENDIAN
    unsigned start_bit: 1;
    unsigned end_bit: 1;
    unsigned reserved: 1;
    unsigned nal_type: 5;
#else
    unsigned nal_type: 5;
    unsigned reserved: 1;
    unsigned end_bit: 1;
    unsigned start_bit: 1;
#endif
};

RtpEncodeH264::RtpEncodeH264(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{}

void RtpEncodeH264::encode(const FrameBuffer::Ptr& frame)
{
    if (_first && _lastPts == frame->dts()) {
        _lastPts += 1;
        _first = false;
    }

    auto size = frame->size() - frame->startSize();
    if (size > _maxRtpSize)  {
        encodeFuA(frame);
    } else {
        encodeSingle(frame);
    }

    if (_enableFastPts) {
        _lastPts = frame->dts() * _ptsScale;
    } else {
        _lastPts = frame->dts();
    }
}

void RtpEncodeH264::encodeFuA(const FrameBuffer::Ptr& frame) {
    auto size = frame->size() - frame->startSize();
    uint64_t pts = _enableFastPts ? frame->dts() * _ptsScale : frame->dts();
    bool first = true;
    auto frameData = frame->data() + frame->startSize();
    auto fuIndicator = (frameData[0] & (~0x1F)) | 28;
    auto fuHeader = (uint8_t)(frameData[0]) & 0x1F;
    FuHeader *fu_flags = (FuHeader *) (&fuHeader);

    // logInfo << "nal type : " << (int)(frameData[0]);
    // logInfo << "fuIndicator : " << fuIndicator;

    size -= 1;
    frameData += 1;

    while (size + 2 > _maxRtpSize) {
        RtpPacket::Ptr rtp = RtpPacket::create(_trackInfo, _maxRtpSize + 12, pts, _ssrc, _lastSeq++, false);
        if (first) { //start
            fu_flags->start_bit = 1;
            first = false;
        } else { //middle
            fu_flags->start_bit = 0;
        }
        
        auto payload = rtp->getPayload();
        payload[0] = fuIndicator;
        payload[1] = fuHeader;
        memcpy(payload + 2, (uint8_t *) frameData, _maxRtpSize - 2);
        size = size + 2 - _maxRtpSize;
        frameData = frameData + _maxRtpSize - 2;

        onRtpPacket(rtp, false);
    }

    if (size > 0) { //end
        // logInfo << "pts: " << pts << ", _lastPts: " << _lastPts;
		if (pts == _lastPts) {
		    logError << "pts == _lastPts";
		}
        RtpPacket::Ptr rtp = RtpPacket::create(_trackInfo, size + 2 + 12, pts, _ssrc, _lastSeq++, true/*pts != _lastPts*/);
        fu_flags->end_bit = 1;
        fu_flags->start_bit = 0;
        auto payload = rtp->getPayload();
        payload[0] = fuIndicator;
        payload[1] = fuHeader;
        memcpy(payload + 2, (uint8_t *) frameData, size);

        onRtpPacket(rtp, false);
    }
}

void RtpEncodeH264::encodeSingle(const FrameBuffer::Ptr& frame) {
    auto size = frame->size() - frame->startSize();
    auto frameData = frame->data() + frame->startSize();
    auto pts = _enableFastPts ? frame->dts() * _ptsScale : frame->dts();

    if (pts == _lastPts) {
        // logError << "pts == _lastPts, " << _lastPts;
    }
    RtpPacket::Ptr rtp;
    if (frame->getNalType() == 7 || frame->getNalType() == 8) {
        rtp = RtpPacket::create(_trackInfo, size + 12, pts, _ssrc, _lastSeq++, false/*pts != _lastPts*/);
    } else {
        rtp = RtpPacket::create(_trackInfo, size + 12, pts, _ssrc, _lastSeq++, pts != _lastPts);
    }
    auto payload = rtp->getPayload();
    // logInfo << "payload size: " << rtp->getPayloadSize();
    // logInfo << "frameData size: " << size;
    memcpy(payload, (uint8_t *) frameData, size);

    onRtpPacket(rtp, frame->startFrame());
}

void RtpEncodeH264::setOnRtpPacket(const function<void(const RtpPacket::Ptr& packet, bool start)>& cb)
{
    _onRtpPacket = cb;
}

void RtpEncodeH264::onRtpPacket(const RtpPacket::Ptr& rtp, bool start)
{
    // logInfo << "encode a h264 rtp time: " << rtp->getStamp();
    if (_onRtpPacket) {
        _onRtpPacket(rtp, start);
    }
}