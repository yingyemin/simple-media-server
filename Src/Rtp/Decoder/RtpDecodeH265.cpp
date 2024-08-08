#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpDecodeH265.h"
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

RtpDecodeH265::RtpDecodeH265(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{
    _frame = createFrame();
}

H265Frame::Ptr RtpDecodeH265::createFrame()
{
    auto frame = make_shared<H265Frame>();
    frame->_startSize = 4;
    frame->_codec = "h265";
    frame->_index = _trackInfo->index_;
    frame->_trackType = VideoTrackType;

    return frame;
}

bool RtpDecodeH265::isStartGop(const RtpPacket::Ptr& rtp)
{
    auto playload = rtp->getPayload();
    int nalType = (playload[0] >> 1) & 0x3f;

    bool start = false;

    switch (nalType) {
        case 48 : {
            // stap A
            auto payload = rtp->getPayload() + 2;
            auto payloadSize = rtp->getPayloadSize() - 2;

            while (payloadSize > 2) {
                auto frameSize = payload[0] < 8 | payload[1];
                if (frameSize == 0) {
                    logWarn << "frame size is 0";
                    continue;
                } else if (payloadSize < frameSize + 2) {
                    logError << "invalid frame data";
                    break;
                }
                int nalType = (playload[2] >> 1) & 0x3f;
                if (nalType >= H265NalType::H265_BLA_W_LP && nalType <= H265NalType::H265_RSV_IRAP_VCL23) {
                    start = true;
                    break;
                }

                payloadSize -= (frameSize + 2);
                payload += (frameSize + 2);
            }
            break;
        }
        case 49 : {
            // fu A
            auto payload = rtp->getPayload();
            FuHeader* header = (FuHeader*)&payload[2];
            if (header->start_bit && header->nal_type >= H265NalType::H265_BLA_W_LP && header->nal_type <= H265NalType::H265_RSV_IRAP_VCL23) {
                start = true;
            }
            break;
        }
        default : {
            // single
            if (nalType < 48) {
                if (nalType >= H265NalType::H265_BLA_W_LP && nalType <= H265NalType::H265_RSV_IRAP_VCL23) {
                    start = true;
                }
            }
            logInfo << "no surpport type";
            break;
        }
    }

    return start;
}

void RtpDecodeH265::decode(const RtpPacket::Ptr& rtp)
{
    if (_firstRtp) {
        _lastSeq = rtp->getSeq() - 1;
        _firstRtp = false;
    }

    auto payloadSize = rtp->getPayloadSize();
    if (payloadSize == 0) {
        logInfo << "payload size is 0";
        return ;
    }

    rtp->samplerate_ = _trackInfo->samplerate_;
    auto playload = rtp->getPayload();
    int nalType = (playload[0] >> 1) & 0x3f;

    switch (nalType) {
        case 48 : {
            // stap A
            if (payloadSize < 3) {
                logInfo << "payload size is 1 with stap A";
                break;
            }
            decodeStapA(rtp);
            break;
        }
        case 49 : {
            // fu A
            decodeFuA(rtp);
            break;
        }
        default : {
            // single
            if (nalType < 48) {
                decodeSingle(rtp->getPayload(), rtp->getPayloadSize(), rtp->getStampMS());
            }
            logInfo << "no surpport type";
            break;
        }
    }
    _lastSeq = rtp->getSeq();
}

void RtpDecodeH265::decodeSingle(const uint8_t *ptr, ssize_t size, uint64_t stamp)
{
    _frame->_buffer.assign("\x00\x00\x00\x01", 4);
    _frame->_buffer.append((char *) ptr, size);
    _frame->_pts = stamp;

    onFrame(_frame);
}

void RtpDecodeH265::decodeStapA(const RtpPacket::Ptr& rtp)
{
    auto stamp = rtp->getStampMS();
    auto payload = rtp->getPayload() + 2;
    auto payloadSize = rtp->getPayloadSize() - 2;

    while (payloadSize > 2) {
        auto frameSize = payload[0] < 8 | payload[1];
        if (frameSize == 0) {
            logWarn << "frame size is 0";
            continue;
        } else if (payloadSize < frameSize + 2) {
            logError << "invalid frame data";
            break;
        }
        decodeSingle(payload + 2, frameSize, stamp);

        payloadSize -= (frameSize + 2);
        payload += (frameSize + 2);
    }
}

void RtpDecodeH265::decodeFuA(const RtpPacket::Ptr& rtp)
{
    auto payload = rtp->getPayload();
    auto payloadSize = rtp->getPayloadSize();
    // uint16_t* indicator = (uint16_t*)payload;
    FuHeader* header = (FuHeader*)&payload[2];

    rtp->samplerate_ = _trackInfo->samplerate_;
    auto stamp = rtp->getStampMS();
    auto seq = rtp->getSeq();

    if (header->start_bit) {
        if (_stage != 0) {
            _frame->_buffer.clear();
        }
        _stage = 1;
        _frame->_buffer.assign("\x00\x00\x00\x01", 4);
        _frame->_buffer.push_back((payload[0] & 0x81) | (header->nal_type << 1));
        _frame->_buffer.push_back(payload[1]);
        _frame->_pts = stamp;
        _frame->_buffer.append((char *) payload + 3, payloadSize - 3);
        if (header->end_bit || rtp->getHeader()->mark) {
            _stage = 0;
            onFrame(_frame);
        }
        return ;
    }

    if (_stage != 1) {
        return ;
    }

    if (seq != (uint16_t)(_lastSeq + 1)) {
        logError << "loss rtp, seq: " << seq << ", _lastSeq: " << _lastSeq;
        _stage = 2;
        return ;
    }

    _frame->_buffer.append((char *) payload + 3, payloadSize - 3);
    if (header->end_bit) {
        _stage = 0;
        onFrame(_frame);
    }
}

void RtpDecodeH265::setOnDecode(const function<void(const FrameBuffer::Ptr& frame)> cb)
{
    _onFrame = cb;
}

void RtpDecodeH265::onFrame(const FrameBuffer::Ptr& frame)
{
    // TODO 存在B帧时如何处理。可参考ffmpeg
    frame->_dts = frame->_pts;
    frame->_index = _trackInfo->index_;
    if (_onFrame) {
        _onFrame(frame);
    }
    _frame = createFrame();
//     FILE* fp = fopen("test1.h265", "ab+");
//     fwrite(frame->data(), 1, frame->size(), fp);
//     fclose(fp);
}