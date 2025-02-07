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

    // _useStapA = true;
    // if (_useStapA && frame->getNalType() == 5) {
    //     FrameBuffer::Ptr sps;
    //     FrameBuffer::Ptr pps;
    //     FrameBuffer::Ptr vps;
    //     _trackInfo->getVpsSpsPps(vps, sps, pps);

    //     int spsSize = sps->size() - sps->startSize();
    //     auto spsData = sps->data() + sps->startSize();
    //     auto rtpSps = RtpPacket::create(_trackInfo, spsSize + 12 + 3, pts, _ssrc, _lastSeq++, false);
        
    //     auto payloadSps = rtpSps->getPayload();
    //     payloadSps[0] = (spsData[0] & (~0x1F)) | 24;
    //     payloadSps[1] = (spsSize >> 8) & 0xFF;
    //     payloadSps[2] = spsSize & 0xff;
    //     memcpy(payloadSps + 3, (uint8_t *) spsData, spsSize);
    //     onRtpPacket(rtpSps, false);

    //     int ppsSize = pps->size() - pps->startSize();
    //     auto ppsData = pps->data() + pps->startSize();
    //     auto rtpPps = RtpPacket::create(_trackInfo, ppsSize + 12 + 3, pts, _ssrc, _lastSeq++, false);
        
    //     auto payloadPps = rtpPps->getPayload();
    //     payloadPps[0] = (ppsData[0] & (~0x1F)) | 24;
    //     payloadPps[1] = (ppsSize >> 8) & 0xFF;
    //     payloadPps[2] = ppsSize & 0xff;
    //     memcpy(payloadPps + 3, (uint8_t *) ppsData, ppsSize);
    //     onRtpPacket(rtpPps, false);
    // }

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

        onRtpPacket(rtp, frame->startFrame());
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

        onRtpPacket(rtp, frame->startFrame());
    }
}

#if 0
void RtpEncodeH264::encodeSingle(const FrameBuffer::Ptr& frame) {
    auto size = frame->size() - frame->startSize();
    auto frameData = frame->data() + frame->startSize();
    auto pts = _enableFastPts ? frame->dts() * _ptsScale : frame->dts();

    if (pts == _lastPts) {
        // logError << "pts == _lastPts, " << _lastPts;
    }

    _useStapA = true;
    if (_useStapA && frame->getNalType() == 5) {
        FrameBuffer::Ptr sps;
        FrameBuffer::Ptr pps;
        FrameBuffer::Ptr vps;
        _trackInfo->getVpsSpsPps(vps, sps, pps);

        int spsSize = sps->size() - sps->startSize();
        auto spsData = sps->data() + sps->startSize();
        auto rtpSps = RtpPacket::create(_trackInfo, spsSize + 12 + 3, pts, _ssrc, _lastSeq++, false);
        
        auto payloadSps = rtpSps->getPayload();
        payloadSps[0] = (spsData[0] & (~0x1F)) | 24;
        payloadSps[1] = (spsSize >> 8) & 0xFF;
        payloadSps[2] = spsSize & 0xff;
        memcpy(payloadSps + 3, (uint8_t *) spsData, spsSize);
        onRtpPacket(rtpSps, false);

        int ppsSize = pps->size() - pps->startSize();
        auto ppsData = pps->data() + pps->startSize();
        auto rtpPps = RtpPacket::create(_trackInfo, ppsSize + 12 + 3, pts, _ssrc, _lastSeq++, false);
        
        auto payloadPps = rtpPps->getPayload();
        payloadPps[0] = (ppsData[0] & (~0x1F)) | 24;
        payloadPps[1] = (ppsSize >> 8) & 0xFF;
        payloadPps[2] = ppsSize & 0xff;
        memcpy(payloadPps + 3, (uint8_t *) ppsData, ppsSize);
        onRtpPacket(rtpPps, false);
    }

    RtpPacket::Ptr rtp;
    if (frame->getNalType() == 7 || frame->getNalType() == 8) {
        rtp = RtpPacket::create(_trackInfo, size + 12, pts, _ssrc, _lastSeq++, true/*pts != _lastPts*/);
    } else {
        rtp = RtpPacket::create(_trackInfo, size + 12, pts, _ssrc, _lastSeq++, pts != _lastPts);
    }
    auto payload = rtp->getPayload();
    // logInfo << "payload size: " << rtp->getPayloadSize();
    // logInfo << "frameData size: " << size;
    memcpy(payload, (uint8_t *) frameData, size);

    onRtpPacket(rtp, frame->startFrame());
}

#else 
void RtpEncodeH264::encodeSingle(const FrameBuffer::Ptr& frame) {
    auto size = frame->size() - frame->startSize();
    auto frameData = frame->data() + frame->startSize();
    auto pts = _enableFastPts ? frame->dts() * _ptsScale : frame->dts();

    if (pts == _lastPts) {
        // logError << "pts == _lastPts, " << _lastPts;
    }
    RtpPacket::Ptr rtp;
    RtpPacket::Ptr rtpDump;
    if (frame->getNalType() == 7 || frame->getNalType() == 8) {
        // _useStapA = true;
        if (_useStapA) {
            rtpDump = RtpPacket::create(_trackInfo, size + 12 + 3, pts, _ssrc, _lastSeq++, false);
            auto payloadDump = rtpDump->getPayload();
            payloadDump[0] = (frameData[0] & (~0x1F)) | 24;
            payloadDump[1] = (size >> 8) & 0xFF;
            payloadDump[2] = size & 0xff;

            memcpy(payloadDump + 3, (uint8_t *) frameData, size);
            _useStapA = false;

            // onRtpPacket(rtpDump, frame->startFrame());
        }
        rtp = RtpPacket::create(_trackInfo, size + 12, pts, _ssrc, _lastSeq++, false/*pts != _lastPts*/);
    } else {
        rtp = RtpPacket::create(_trackInfo, size + 12, pts, _ssrc, _lastSeq++, pts != _lastPts);
    }
    auto payload = rtp->getPayload();
    // logInfo << "payload size: " << rtp->getPayloadSize();
    // logInfo << "frameData size: " << size;
    memcpy(payload, (uint8_t *) frameData, size);

    // 必须乱序，否则webrtc部分视频无法播放
    onRtpPacket(rtp, frame->startFrame());
    if (rtpDump) {
        onRtpPacket(rtpDump, frame->startFrame());
    }
}
#endif

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