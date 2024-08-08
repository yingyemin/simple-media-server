#include "RtmpDecodeCommon.h"
#include "Logger.h"

using namespace std;

RtmpDecodeCommon::RtmpDecodeCommon(const shared_ptr<TrackInfo>& trackInfo)
{
    _trackInfo = trackInfo;
}

RtmpDecodeCommon::~RtmpDecodeCommon()
{
}

void RtmpDecodeCommon::decode(const RtmpMessage::Ptr& msg)
{
    uint8_t *payload = (uint8_t *)msg->payload.get();
    int length = msg->length;
    auto frame = make_shared<FrameBuffer>();
        
    frame->_codec = _trackInfo->codec_;
    frame->_index = _trackInfo->index_;
    frame->_trackType = AudioTrackType;
    frame->_dts = frame->_pts = msg->abs_timestamp;

    frame->_buffer.append((char*)payload + 1, length - 1);
    onFrame(frame);
}

void RtmpDecodeCommon::setOnFrame(const function<void(const FrameBuffer::Ptr& frame)> cb)
{
    _onFrame = cb;
}

void RtmpDecodeCommon::onFrame(const FrameBuffer::Ptr& frame)
{
    if (_onFrame) {
        _onFrame(frame);
    }
}
