#include "RtmpDecodeAac.h"
#include "Logger.h"
#include "Codec/AacFrame.h"

using namespace std;

RtmpDecodeAac::RtmpDecodeAac(const shared_ptr<TrackInfo>& trackInfo)
{
    _trackInfo = dynamic_pointer_cast<AacTrack>(trackInfo);
}

RtmpDecodeAac::~RtmpDecodeAac()
{
}

void RtmpDecodeAac::decode(const RtmpMessage::Ptr& msg)
{
    uint8_t *payload = (uint8_t *)msg->payload->data();
    int length = msg->length;
    if (_first && payload[1] == 0) {
        _aacConfig.assign((char*)payload + 2, length - 2); 
        _trackInfo->setAacInfo(_aacConfig);
        _first = false;
    } else {
        // i b p
        auto frame = make_shared<AacFrame>();
        
        frame->_startSize = 7;
        frame->_codec = "aac";
        frame->_index = _trackInfo->index_;
        frame->_trackType = AudioTrackType;
        frame->_dts = frame->_pts = msg->abs_timestamp;

        frame->_buffer->assign(_trackInfo->getAdtsHeader(length - 2));

        frame->_buffer->append((char*)payload + 2, length - 2);
        onFrame(frame);
    }
}

void RtmpDecodeAac::setOnFrame(const function<void(const FrameBuffer::Ptr& frame)>& cb)
{
    _onFrame = cb;
}

void RtmpDecodeAac::onFrame(const FrameBuffer::Ptr& frame)
{
    if (_onFrame) {
        _onFrame(frame);
    }
}
