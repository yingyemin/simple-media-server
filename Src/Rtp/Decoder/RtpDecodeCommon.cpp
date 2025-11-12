#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpDecodeCommon.h"
#include "Logger.h"
#include "Util/String.hpp"

using namespace std;

RtpDecodeCommon::RtpDecodeCommon(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{
    _frame = createFrame();
}

FrameBuffer::Ptr RtpDecodeCommon::createFrame()
{
    auto frame = make_shared<FrameBuffer>();
    frame->_startSize = 0;
    frame->_codec = _trackInfo->codec_;
    frame->_index = _trackInfo->index_;
    frame->_trackType = _trackInfo->trackType_ == "video" ? VideoTrackType : AudioTrackType;

    return frame;
}

void RtpDecodeCommon::decode(const RtpPacket::Ptr& rtp)
{
    auto payloadSize = rtp->getPayloadSize();
    if (payloadSize == 0) {
        //无实际负载
        return ;
    }

    rtp->samplerate_ = _trackInfo->samplerate_;
    auto payload = rtp->getPayload();
    auto stamp = rtp->getStampMS();
    auto seq = rtp->getSeq();
    auto size = rtp->getPayloadSize();

    if (_firstRtp) {
        _frame->_pts = stamp;
        _frame->_buffer->assign((char *) payload, payloadSize);
        _firstRtp = false;
        _lastStamp = stamp;
        _lastSeq = seq;
        _stage = 1;
        return ;
    }

    if ((_lastStamp != stamp || 
            (size > 4 && (uint8_t)payload[0] == 0x00 && 
            (uint8_t)payload[1] == 0x00 && 
            (uint8_t)payload[2] == 0x01 && 
            (uint8_t)payload[3] == 0xba)) 
        && _frame->size() > 0) {
        if (_stage == 1) {
            onFrame(_frame);
        } else {
            _frame = createFrame();
        }

        _frame->_pts = stamp;
        _frame->_buffer->assign((char *) payload, payloadSize);
        _stage = 1;
        
        _lastStamp = stamp;
        _lastSeq = seq;
        return ;
    }

    if (_stage != 1) {
        _lastStamp = stamp;
        _lastSeq = seq;
        return ;
    }

    if (seq != (uint16_t)(_lastSeq + 1)) {
        logError << "loss rtp, seq: " << seq << ", _lastSeq: " << _lastSeq;
        _stage = 2;
        
        _lastStamp = stamp;
        _lastSeq = seq;
        return ;
    }

    _lastStamp = stamp;
    _lastSeq = seq;
    _frame->_buffer->append((char *) payload, payloadSize);

    if (rtp->getHeader()->mark) {
        onFrame(_frame);
    }
}

void RtpDecodeCommon::setOnDecode(const function<void(const FrameBuffer::Ptr& frame)> cb)
{
    _onFrame = cb;
}

void RtpDecodeCommon::onFrame(const FrameBuffer::Ptr& frame)
{
    // TODO aac_cfg
    frame->_dts = frame->_pts;
    frame->_index = _trackInfo->index_;
    if (_onFrame) {
        _onFrame(frame);
    }
    
    // FILE* fp = fopen("test1.aac", "ab+");
    // fwrite(frame->data(), 1, frame->size(), fp);
    // fclose(fp);

    _frame = createFrame();
}