#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpDecodeH264.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "Common/Config.h"

using namespace std;

#pragma pack(push, 1)

class FuHeader {
public:
#if __BYTE_ORDER == __BIG_ENDIAN
    unsigned char start_bit: 1;
    unsigned char end_bit: 1;
    unsigned char reserved: 1;
    unsigned char nal_type: 5;
#else
    unsigned char nal_type: 5;
    unsigned char reserved: 1;
    unsigned char end_bit: 1;
    unsigned char start_bit: 1;
#endif
};

#pragma pack(pop)

RtpDecodeH264::RtpDecodeH264(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{
    _frame = createFrame();
}

H264Frame::Ptr RtpDecodeH264::createFrame()
{
    auto frame = make_shared<H264Frame>();
    frame->_startSize = 4;
    frame->_codec = "h264";
    frame->_index = _trackInfo->index_;
    frame->_trackType = VideoTrackType;

    return frame;
}

bool RtpDecodeH264::isStartGop(const RtpPacket::Ptr& rtp)
{
    auto playload = rtp->getPayload();
    int nalType = playload[0] & 0x1F;

    bool start = false;

    switch (nalType) {
        case 5:{
            // single
            start = true;
            break;
        }
        case 24 : {
            // stap A
            auto payload = rtp->getPayload() + 1;
            auto payloadSize = rtp->getPayloadSize() -1;

            // logInfo << "payload size:" << payloadSize;

            while (payloadSize > 2) {
                auto frameSize = payload[0] << 8 | payload[1];
                // logInfo << "frame size:" << frameSize;
                if (frameSize == 0) {
                    logWarn << "frame size is 0";
                    continue;
                } else if (payloadSize < frameSize + 2) {
                    logError << "invalid frame data, payloadSize: " << payloadSize << ", frameSize + 2 : " << (frameSize + 2);
                    break;
                }
                int nalType = playload[2] & 0x1F;
                if (nalType == H264_IDR) {
                    start = true;
                    break;
                }

                payloadSize -= (frameSize + 2);
                payload += (frameSize + 2);
            }
            break;
        }
        case 28 : {
            // fu A
            auto payload = rtp->getPayload();
            FuHeader* header = (FuHeader*)(&payload[1]);
            if (header->start_bit && header->nal_type == H264_IDR) {
                start = true;
            }
            break;

        }
        default : {
            // logInfo << "unknown nal type";
            
            break;
        }
    }

    return start;
}

void RtpDecodeH264::decode(const RtpPacket::Ptr& rtp)
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
    int nalType = playload[0] & 0x1F;

    switch (nalType) {
        case 1:
        case 5:
        case 6:
        case 7:
        case 8:{
            // single
            decodeSingle(rtp->getPayload(), rtp->getPayloadSize(), rtp->getStampMS());
            break;
        }
        case 24 : {
            // stap A
            if (payloadSize == 1) {
                logInfo << "payload size is 1 with stap A";
                break;
            }
            decodeStapA(rtp);
            break;
        }
        case 28 : {
            // fu A
            decodeFuA(rtp);
            break;
        }
        default : {
            logInfo << "unknown nal type";
            
            break;
        }
    }
    _lastSeq = rtp->getSeq();
    _lastStamp = rtp->getStampMS();
}

void RtpDecodeH264::decodeSingle(const uint8_t *ptr, ssize_t size, uint64_t stamp)
{
    _frame->_buffer->assign("\x00\x00\x00\x01", 4);
    _frame->_buffer->append((char *) ptr, size);
    _frame->_pts = stamp;

    onFrame(_frame);
}

void RtpDecodeH264::decodeStapA(const RtpPacket::Ptr& rtp)
{
    auto stamp = rtp->getStampMS();
    auto payload = rtp->getPayload() + 1;
    auto payloadSize = rtp->getPayloadSize() -1;

    // logInfo << "payload size:" << payloadSize;

    while (payloadSize > 2) {
        auto frameSize = payload[0] << 8 | payload[1];
        // logInfo << "frame size:" << frameSize;
        if (frameSize == 0) {
            logWarn << "frame size is 0";
            continue;
        } else if (payloadSize < frameSize + 2) {
            logError << "invalid frame data, payloadSize: " << payloadSize << ", frameSize + 2 : " << (frameSize + 2);
            break;
        }
        decodeSingle(payload + 2, frameSize, stamp);

        payloadSize -= (frameSize + 2);
        payload += (frameSize + 2);
    }
}

void RtpDecodeH264::decodeFuA(const RtpPacket::Ptr& rtp)
{
    auto payload = rtp->getPayload();
    auto payloadSize = rtp->getPayloadSize();
    uint8_t indicator = payload[0]  & (~0x1F);

    FuHeader header;
    header.start_bit = (payload[1] & 0b10000000) >> 7;
    header.end_bit = (payload[1] & 0b01000000) >> 6;
    header.reserved = (payload[1] & 0b00100000) >> 5;
    header.nal_type = payload[1] & 0b00011111;

    // rtp->samplerate_ = _trackInfo->samplerate_;
    auto stamp = rtp->getStampMS();
    auto seq = rtp->getSeq();

    if (header.start_bit || _lastStamp != stamp) {
        if (_stage != 0) {
            _frame->_buffer->clear();
        }
        // logInfo << "start a frame: " << (int)header->nal_type;
        _stage = 1;
        _frame->_buffer->assign("\x00\x00\x00\x01", 4);
        _frame->_buffer->push_back(indicator | header.nal_type);
        _frame->_pts = stamp;
        _frame->_buffer->append((char *) payload + 2, payloadSize - 2);
        if (header.end_bit || rtp->getHeader()->mark) {
            _stage = 0;
            onFrame(_frame);
        }
        return ;
    }

    if (_stage != 1) {
        return ;
    }

    if (seq != (uint16_t)(_lastSeq + 1)) {
        logError << "loss rtp, last seq: " << _lastSeq << ", cur seq: " << seq;
        _stage = 2;
        return ;
    }

    _frame->_buffer->append((char *) payload + 2, payloadSize - 2);
    if (header.end_bit) {
        _stage = 0;
        onFrame(_frame);
    }
}

void RtpDecodeH264::setOnDecode(const function<void(const FrameBuffer::Ptr& frame)> cb)
{
    _onFrame = cb;
}

void RtpDecodeH264::onFrame(const FrameBuffer::Ptr& frame)
{
    // TODO 存在B帧时如何处理。可参考ffmpeg

    auto payload = frame->data() + 4;
    // logInfo << "decode a h264 frame type: " << (int)(payload[0] & 0x1F);
    // logInfo << "decode a h264 frame size: " << frame->size();
    // logInfo << "decode a h264 frame time: " << frame->pts();
    static int enableDtsGenerator = Config::instance()->getAndListen([](const json &config){
        enableDtsGenerator = Config::instance()->get("Rtp", "enableDtsGenerator");
    }, "Rtp", "enableDtsGenerator");
    
    if (enableDtsGenerator) {
        _dtsGenerator.getDts(frame->_pts, frame->_dts);
    } else {
        frame->_dts = frame->_pts;
    }
    if (_onFrame) {
        _onFrame(frame);
    }
    _frame = createFrame();
    // FILE* fp = fopen("test1.h264", "ab+");
    // fwrite(frame->data(), 1, frame->size(), fp);
    // fclose(fp);
}