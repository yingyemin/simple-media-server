#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpDecodeMp3.h"
#include "Logger.h"
#include "Util/String.hpp"

using namespace std;

RtpDecodeMp3::RtpDecodeMp3(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{
    _frame = createFrame();
}

FrameBuffer::Ptr RtpDecodeMp3::createFrame()
{
    auto frame = make_shared<FrameBuffer>();
    frame->_startSize = 0;
    frame->_codec = _trackInfo->codec_;
    frame->_index = _trackInfo->index_;
    frame->_trackType = AudioTrackType;

    return frame;
}

void RtpDecodeMp3::decode(const RtpPacket::Ptr& rtp)
{
    auto payloadSize = rtp->getPayloadSize();
    if (payloadSize == 0) {
        //无实际负载
        return ;
    }

    rtp->samplerate_ = 90000;//_trackInfo->samplerate_;
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

        if (rtp->getHeader()->mark) {
            onFrame(_frame);
        }
        
        return ;
    }

    if ((_lastStamp != stamp) && _frame->size() > 0) {
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

void RtpDecodeMp3::setOnDecode(const function<void(const FrameBuffer::Ptr& frame)> cb)
{
    _onFrame = cb;
}

void RtpDecodeMp3::onFrame(const FrameBuffer::Ptr& frame)
{
    if (frame->size() > 4) {
        frame->_dts = frame->_pts;
        frame->_index = _trackInfo->index_;
        frame->_buffer->substr(4);
        if (_onFrame) {
            _onFrame(frame);
        }
    }

    // logInfo << "pts is : " << frame->_pts;
    
    // FILE* fp = fopen("test1.aac", "ab+");
    // fwrite(frame->data(), 1, frame->size(), fp);
    // fclose(fp);

    _frame = createFrame();
}