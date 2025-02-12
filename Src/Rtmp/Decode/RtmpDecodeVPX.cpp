#include "RtmpDecodeVPX.h"
#include "Logger.h"
#include "Common/Frame.h"

using namespace std;

RtmpDecodeVPX::RtmpDecodeVPX(const shared_ptr<TrackInfo>& trackInfo)
{
    _trackInfo = trackInfo;
}

RtmpDecodeVPX::~RtmpDecodeVPX()
{
}

FrameBuffer::Ptr RtmpDecodeVPX::createFrame()
{
    FrameBuffer::Ptr frame;
    if (_trackInfo->codec_ == "vp8") {
        frame = make_shared<VP8Frame>();
    } else {
        frame = make_shared<VP9Frame>();
    }
    frame->_startSize = 0;
    frame->_codec = _trackInfo->codec_;
    frame->_index = _trackInfo->index_;
    frame->_trackType = VideoTrackType;

    return frame;
}

void RtmpDecodeVPX::decode(const RtmpMessage::Ptr& msg)
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
        // rtmp header 5 bytes, VPX header 8 bytes
        if (length < 9) {
            return ;
        }

        // auto frame = FrameBuffer::createFrame(_trackInfo->codec_, 0, _trackInfo->index_, false);
        // frame->_pts = frame->_dts = msg->abs_timestamp;
        // frame->_buffer.append((char*)payload + 5, length - 5);
        // _trackInfo->setVps(frame);
        auto vp9Track = dynamic_pointer_cast<VP9Track>(_trackInfo);
        vp9Track->setConfig(string((char*)payload + 5 + 4, length - 5 - 4));
        // onFrame(frame);
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
            auto frame = FrameBuffer::createFrame(_trackInfo->codec_, 0, _trackInfo->index_, false);
            frame->_pts = frame->_dts = msg->abs_timestamp;
            frame->_pts += cts;
            frame->_buffer.append((char*)payload, end - payload);
            
            onFrame(frame);
        }
    }
}

void RtmpDecodeVPX::setOnFrame(const function<void(const FrameBuffer::Ptr& frame)> cb)
{
    _onFrame = cb;
}

void RtmpDecodeVPX::onFrame(const FrameBuffer::Ptr& frame)
{
    // logInfo << "get a VPX nal: " << (int)(frame->getNalType());
    if (_onFrame) {
        _onFrame(frame);
    }
}
