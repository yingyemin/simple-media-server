#include "RtmpDecodeH265.h"
#include "Logger.h"
#include "Common/Frame.h"

using namespace std;

RtmpDecodeH265::RtmpDecodeH265(const shared_ptr<TrackInfo>& trackInfo)
{
    _trackInfo = dynamic_pointer_cast<H265Track>(trackInfo);
}

RtmpDecodeH265::~RtmpDecodeH265()
{
}

H265Frame::Ptr RtmpDecodeH265::createFrame()
{
    auto frame = make_shared<H265Frame>();
    frame->_startSize = 4;
    frame->_codec = "h265";
    frame->_index = _trackInfo->index_;
    frame->_trackType = VideoTrackType;

    frame->_buffer.assign("\x00\x00\x00\x01", 4);

    return frame;
}

void RtmpDecodeH265::decode(const RtmpMessage::Ptr& msg)
{
    uint8_t *payload = (uint8_t *)msg->payload->data();
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
        // rtmp header 5 bytes, hvcc 22 bytes
        if (length < 28) {
            return ;
        }

        int numOfArr = payload[27];
        int index = 28;
        logInfo << "numOfArr is : " << numOfArr;
        for (int i = 0; i < numOfArr; ++i) {
            int len =(payload[index + 1] & 0xFF) << 8 | (payload[index + 2] & 0xFF);
            index += 3;
            for (int j = 0; j < len; ++j) {
                int naluLen =(payload[index] & 0xFF) << 8 | (payload[index + 1] & 0xFF);
                index += 2;

                auto frame = createFrame();
                frame->_pts = frame->_dts = msg->abs_timestamp;
                frame->_buffer.append((char*)payload + index, naluLen);
                index += naluLen;

                logInfo << "get a config frame: " << (int)frame->getNalType();
                if (frame->getNalType() == H265_VPS) {
                    _trackInfo->setVps(frame);
                } else if (frame->getNalType() == H265_SPS) {
                    _trackInfo->setSps(frame);
                } else if (frame->getNalType() == H265_PPS) {
                    _trackInfo->setPps(frame);
                }

                onFrame(frame);
            }
        }
    } else {
        // i b p
        int len =0;
        int num =5;
        int32_t cts = 0;

        if (isEnhance) {
            if (packet_type == 1 || packet_type == 3) {
                if (packet_type == 1) {
                    cts = (((payload[num] << 16) | (payload[num + 1] << 8) | (payload[num + 2])) + 0xff800000) ^ 0xff800000;
                    num += 3;
                }
            } else {
                return ;
            }
        }

        while(num < length) {

            len = (payload[num] & 0x000000FF) << 24 | (payload[num+1] & 0x000000FF) << 16 | 
                    (payload[num+2] & 0x000000FF) << 8 | payload[num+3] & 0x000000FF;

            num += 4;

            auto frame = createFrame();
            frame->_pts = frame->_dts = msg->abs_timestamp;
            frame->_pts += cts;
            frame->_buffer.append((char*)payload + num, len);
            
            onFrame(frame);

            num += len;
        }
    }
}

void RtmpDecodeH265::setOnFrame(const function<void(const FrameBuffer::Ptr& frame)> cb)
{
    _onFrame = cb;
}

void RtmpDecodeH265::onFrame(const FrameBuffer::Ptr& frame)
{
    logInfo << "get a h265 nal: " << (int)(frame->getNalType());
    if (_onFrame) {
        _onFrame(frame);
    }
}
