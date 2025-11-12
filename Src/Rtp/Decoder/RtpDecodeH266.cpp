#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpDecodeH266.h"
#include "Logger.h"
#include "Util/String.hpp"

using namespace std;

class FuHeader {
public:
#if __BYTE_ORDER == __BIG_ENDIAN
    unsigned start_bit: 1;
    unsigned end_bit: 1;
    unsigned mark: 1;
    unsigned nal_type: 5;
#else
    unsigned nal_type: 5;
    unsigned mark: 1;
    unsigned end_bit: 1;
    unsigned start_bit: 1;
#endif
};

RtpDecodeH266::RtpDecodeH266(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{
    _frame = createFrame();
}

H266Frame::Ptr RtpDecodeH266::createFrame()
{
    auto frame = make_shared<H266Frame>();
    frame->_startSize = 4;
    frame->_codec = "h266";
    frame->_index = _trackInfo->index_;
    frame->_trackType = VideoTrackType;

    return frame;
}

bool RtpDecodeH266::isStartGop(const RtpPacket::Ptr& rtp)
{
    auto playload = rtp->getPayload();
    int nalType = (playload[1] >> 3) & 0x1f;

    bool start = false;

    switch (nalType) {
        case 28 : {
            // stap A
            auto payload = rtp->getPayload() + 2;
            auto payloadSize = rtp->getPayloadSize() - 2;

            while (payloadSize > 2) {
                auto frameSize = payload[0] << 8 | payload[1];
                if (frameSize == 0) {
                    logWarn << "frame size is 0";
                    continue;
                } else if (payloadSize < frameSize + 2) {
                    logError << "invalid frame data";
                    break;
                }
                int nalType = (playload[2] >> 1) & 0x3f;
                if (nalType >= H266NalType::H266_IDR_W_RADL && nalType < H266NalType::H266_RSV_IRAP) {
                    start = true;
                    break;
                }

                payloadSize -= (frameSize + 2);
                payload += (frameSize + 2);
            }
            break;
        }
        case 29 : {
            // fu A
            auto payload = rtp->getPayload();
            FuHeader* header = (FuHeader*)&payload[2];
            if (header->start_bit && header->nal_type >= H266NalType::H266_IDR_W_RADL && header->nal_type < H266NalType::H266_RSV_IRAP) {
                start = true;
            }
            break;
        }
        default : {
            // single
            if (nalType < 28) {
                if (nalType >= H266NalType::H266_IDR_W_RADL && nalType < H266NalType::H266_RSV_IRAP) {
                    start = true;
                }
            }
            logInfo << "no surpport type";
            break;
        }
    }

    return start;
}

void RtpDecodeH266::decode(const RtpPacket::Ptr& rtp)
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
    int nalType = (playload[1] >> 3) & 0x1f;

    switch (nalType) {
        case 28 : {
            // stap A
            if (payloadSize < 3) {
                logInfo << "payload size is 1 with stap A";
                break;
            }
            decodeStapA(rtp);
            break;
        }
        case 29 : {
            // fu A
            decodeFuA(rtp);
            break;
        }
        default : {
            // single
            if (nalType < 28) {
                decodeSingle(rtp->getPayload(), rtp->getPayloadSize(), rtp->getStampMS());
            } else {
                logDebug << "no surpport type";
            }
            break;
        }
    }
    _lastSeq = rtp->getSeq();
}

void RtpDecodeH266::decodeSingle(const uint8_t *ptr, ssize_t size, uint64_t stamp)
{
    _frame->_buffer->assign("\x00\x00\x00\x01", 4);
    _frame->_buffer->append((char *) ptr, size);
    _frame->_pts = stamp;

    onFrame(_frame);
}

void RtpDecodeH266::decodeStapA(const RtpPacket::Ptr& rtp)
{
    auto stamp = rtp->getStampMS();
    auto payload = rtp->getPayload() + 2;
    auto payloadSize = rtp->getPayloadSize() - 2;

    while (payloadSize > 2) {
        auto frameSize = payload[0] << 8 | payload[1];
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

void RtpDecodeH266::decodeFuA(const RtpPacket::Ptr& rtp)
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
            _frame->_buffer->clear();
        }
        _stage = 1;
        _frame->_buffer->assign("\x00\x00\x00\x01", 4);
        _frame->_buffer->push_back(payload[0]);
        _frame->_buffer->push_back(((payload[2] & 0x1f) << 3) & (payload[1] & 0x07));
        _frame->_pts = stamp;
        _frame->_buffer->append((char *) payload + 3, payloadSize - 3);
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

    _frame->_buffer->append((char *) payload + 3, payloadSize - 3);
    if (header->end_bit) {
        _stage = 0;
        onFrame(_frame);
    }
}

void RtpDecodeH266::setOnDecode(const function<void(const FrameBuffer::Ptr& frame)> cb)
{
    _onFrame = cb;
}

void RtpDecodeH266::onFrame(const FrameBuffer::Ptr& frame)
{
    // TODO 存在B帧时如何处理。可参考ffmpeg
    // logInfo << "frame size: " << frame->size() << ", pts: " << frame->pts() << ", type: " << (int)frame->getNalType();
    frame->_dts = frame->_pts;
    frame->_index = _trackInfo->index_;
    if (_onFrame) {
        _onFrame(frame);
    }
    _frame = createFrame();
//     FILE* fp = fopen("test1.H266", "ab+");
//     fwrite(frame->data(), 1, frame->size(), fp);
//     fclose(fp);
}