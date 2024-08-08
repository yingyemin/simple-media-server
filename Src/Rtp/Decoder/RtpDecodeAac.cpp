#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpDecodeAac.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

RtpDecodeAac::RtpDecodeAac(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(static_pointer_cast<AacTrack>(trackInfo))
{
    _frame = createFrame();
}

FrameBuffer::Ptr RtpDecodeAac::createFrame()
{
    auto frame = make_shared<FrameBuffer>();
    frame->_startSize = 7;
    frame->_codec = "aac";
    frame->_index = _trackInfo->index_;
    frame->_trackType = AudioTrackType;

    return frame;
}

void RtpDecodeAac::decode(const RtpPacket::Ptr& rtp)
{
    auto payloadSize = rtp->getPayloadSize();
    if (payloadSize == 0) {
        //无实际负载
        return ;
    }

    rtp->samplerate_ = _trackInfo->samplerate_;
    auto payload = rtp->getPayload();
    auto stamp = rtp->getStampMS();

    auto au_header_count = ((payload[0] << 8) | payload[1]) >> 4;
    if (!au_header_count) {
        //问题issue: https://github.com/ZLMediaKit/ZLMediaKit/issues/1869
        logInfo << "invalid aac rtp au_header_count";
        return ;
    }

    if (payloadSize < (au_header_count * 2 + 2)) {
        logInfo << "invalid aac rtp length";
        return ;
    }

    auto au_header_ptr = payload + 2;
    payload += au_header_count * 2 + 2;
    payloadSize -= (au_header_count * 2 + 2);

    auto step = 0;
    if (_firstRtp) {
        step = 0;
        _firstRtp = false;
        _lastStamp = stamp;
    }

    step = (stamp -_lastStamp) / au_header_count;
    if (step < 0) {
        step = 0;
    }

    for (int i = 0; i < au_header_count; ++i) {
        // 之后的2字节是AU_HEADER,其中高13位表示一帧AAC负载的字节长度，低3位无用
        uint16_t size = ((au_header_ptr[0] << 8) | au_header_ptr[1]) >> 3;
        if (size > payloadSize) {
            //数据不够
            break;
        }

        if (size) {
            //设置aac数据
            if (_frame->_buffer.empty()) {
                _frame->_buffer.assign((char *) payload, size);
            } else {
                _frame->_buffer.append((char *) payload, size);
            }
            //设置当前audio unit时间戳
            _frame->_pts = _lastStamp + i * step;
            payload += size;
            payloadSize -= size;
            au_header_ptr += 2;
            if (rtp->getHeader()->mark || _lastStamp != stamp) {
                onFrame(_frame);
            }
        }
    }
    //记录上次时间戳
    _lastStamp = stamp;
}

void RtpDecodeAac::setOnDecode(const function<void(const FrameBuffer::Ptr& frame)> cb)
{
    _onFrame = cb;
}

void RtpDecodeAac::onFrame(const FrameBuffer::Ptr& frame)
{
    // TODO aac_cfg
    auto adts = _trackInfo->getAdtsHeader(frame->size());
    frame->_buffer.insert(0, adts.data(), adts.size());
    frame->_dts = frame->_pts;
    frame->_index = _trackInfo->index_;
    if (_onFrame) {
        _onFrame(frame);
    }
    
    // logInfo << "decode a aac frame: " << frame->size();
    // FILE* fp = fopen("test1.aac", "ab+");
    // fwrite(frame->data(), 1, frame->size(), fp);
    // fclose(fp);

    _frame = createFrame();
}