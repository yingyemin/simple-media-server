#include "RtmpDecodeAV1.h"
#include "Logger.h"
#include "Common/Frame.h"

using namespace std;

RtmpDecodeAV1::RtmpDecodeAV1(const shared_ptr<TrackInfo>& trackInfo)
{
    _trackInfo = dynamic_pointer_cast<AV1Track>(trackInfo);
}

RtmpDecodeAV1::~RtmpDecodeAV1()
{
}

AV1Frame::Ptr RtmpDecodeAV1::createFrame()
{
    auto frame = make_shared<AV1Frame>();
    frame->_startSize = 0;
    frame->_codec = "av1";
    frame->_index = _trackInfo->index_;
    frame->_trackType = VideoTrackType;

    return frame;
}

void RtmpDecodeAV1::decode(const RtmpMessage::Ptr& msg)
{
    uint8_t *payload = (uint8_t *)msg->payload->data();
    auto end = payload + msg->length;
    bool isEnhance = (payload[0] >> 4) & 0b1000;
    uint8_t packet_type;

    if (isEnhance) {
        packet_type = payload[0] & 0x0f;
    } else {
        packet_type = payload[1];
    }

    int length = msg->length;
    if (/*_first && */packet_type == 0) {
        logInfo << "get a flv config";
        // rtmp header 5 bytes, av1 header 4 bytes
        if (length < 9) {
            return ;
        }

        auto frame = FrameBuffer::createFrame("av1", 0, _trackInfo->index_, false);
        frame->_pts = frame->_dts = msg->abs_timestamp;
        frame->_buffer.append((char*)payload + 9, length - 9);
        _trackInfo->setVps(frame);
        onFrame(frame);
    } else {
        // i b p
        int num = 5;
        int32_t cts = 0;

        // if (isEnhance) {
        //     if (packet_type == 1 || packet_type == 3) {
        //         if (packet_type == 1) {
        //             cts = (((payload[num] << 16) | (payload[num + 1] << 8) | (payload[num + 2])) + 0xff800000) ^ 0xff800000;
        //             num += 3;
        //         }
        //     } else {
        //         return ;
        //     }
        // }

        payload += num;
        if(payload < end) {
            auto frame = FrameBuffer::createFrame("av1", 0, _trackInfo->index_, false);
            frame->_pts = frame->_dts = msg->abs_timestamp;
            frame->_pts += cts;
            frame->_buffer.append((char*)payload, end - payload);
            
            onFrame(frame);
        }
    }
}

void RtmpDecodeAV1::setOnFrame(const function<void(const FrameBuffer::Ptr& frame)> cb)
{
    _onFrame = cb;
}

void RtmpDecodeAV1::onFrame(const FrameBuffer::Ptr& frame)
{
    logInfo << "get a av1 nal: " << (int)(frame->getNalType());
    if (_onFrame) {
        _onFrame(frame);
    }
}
