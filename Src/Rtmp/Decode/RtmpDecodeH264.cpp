#include "RtmpDecodeH264.h"
#include "Logger.h"
#include "EventLoopPool.h"
#include "Common/Frame.h"

using namespace std;

RtmpDecodeH264::RtmpDecodeH264(const shared_ptr<TrackInfo>& trackInfo)
{
    _trackInfo = dynamic_pointer_cast<H264Track>(trackInfo);
}

RtmpDecodeH264::~RtmpDecodeH264()
{
}

H264Frame::Ptr RtmpDecodeH264::createFrame()
{
    auto frame = make_shared<H264Frame>();
    frame->_startSize = 4;
    frame->_codec = "h264";
    frame->_index = _trackInfo->index_;
    frame->_trackType = VideoTrackType;

    frame->_buffer.assign("\x00\x00\x00\x01", 4);

    return frame;
}

void RtmpDecodeH264::decode(const RtmpMessage::Ptr& msg)
{
    uint8_t *payload = (uint8_t *)msg->payload->data();
    int stamp = msg->abs_timestamp;

    int length = msg->length;
    if (/*_first && */payload[1] == 0) {
        // sps pps
        // rtmp 头(5) + avcc 前八个字节(8) = 13；第11个字节是sps的数量；12、13字节是sps的长度

        // sps
        if (length < 13) {
            return ;
        }
        int spsNum = payload[10] & 0x1f;
        int spsCount = 0;
        int index = 11;
        while (spsCount < spsNum) {
            // 这里应该判断一下，剩余长度够不够
            int spsLen =(payload[index] & 0x000000FF) << 8 | (payload[index+1] & 0x000000FF);
            index += 2;
            auto frame = createFrame();
            frame->_buffer.append((char*)payload + index, spsLen);
            frame->_pts = frame->_dts = stamp;
            _trackInfo->setSps(frame);

            onFrame(frame);

            index += spsLen;
            ++spsCount;
        }

        // pps
        int ppsNum = payload[index] & 0x1f;
        int ppsCount = 0;
        index += 1;
        while (ppsCount < ppsNum) {
            int ppsLen =(payload[index] & 0x000000FF) << 8 | (payload[index+1] & 0x000000FF);
            index += 2;
            auto frame = createFrame();
            frame->_buffer.append((char*)payload + index, ppsLen);
            frame->_pts = frame->_dts = stamp;

            _trackInfo->setPps(frame);

            onFrame(frame);

            index += ppsLen;
            ++ppsCount;
        }

        // _first = false;
    } else {
        // i b p
        int len =0;
        int num =5;

        while(num < length) {

            len = (payload[num] & 0x000000FF) << 24 | (payload[num+1] & 0x000000FF) << 16 | 
                    (payload[num+2] & 0x000000FF) << 8 | payload[num+3] & 0x000000FF;

            num += 4;

            auto frame = createFrame();
            frame->_buffer.append((char*)payload + num, len);
            frame->_pts = frame->_dts = stamp;
            onFrame(frame);

            // logInfo << "is keyframe: " << frame->keyFrame();

            num += len;
        }
    }
}

void RtmpDecodeH264::setOnFrame(const function<void(const FrameBuffer::Ptr& frame)> cb)
{
    _onFrame = cb;
}

void RtmpDecodeH264::onFrame(const FrameBuffer::Ptr& frame)
{
    if (_onFrame) {
        _onFrame(frame);
    }
}
