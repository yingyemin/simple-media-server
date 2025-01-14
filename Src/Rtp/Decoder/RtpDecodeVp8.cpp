#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpDecodeVp8.h"
#include "Logger.h"
#include "Util/String.h"
// #include "Codec/VP8Frame.h"

using namespace std;

RtpDecodeVp8::RtpDecodeVp8(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{
    _frame = createFrame();
}

FrameBuffer::Ptr RtpDecodeVp8::createFrame()
{
    // auto frame = make_shared<VP8Frame>();
    auto frame = FrameBuffer::createFrame("vp8", 0, _trackInfo->index_, 0);
    // frame->_startSize = 0;
    // frame->_codec = _trackInfo->codec_;
    // frame->_index = _trackInfo->index_;
    // frame->_trackType = VideoTrackType;

    return frame;
}

bool RtpDecodeVp8::isStartGop(const RtpPacket::Ptr& rtp)
{
    auto payload_size = rtp->getPayloadSize();
    if (payload_size <= 0) {
        // 无实际负载  [AUTO-TRANSLATED:305af48f]
        // No actual payload
        return false;
    }
    auto ptr = rtp->getPayload();
    auto stamp = rtp->getStampMS();
    auto seq = rtp->getSeq();
    auto pend = ptr + payload_size;
    /*
         0 1 2 3 4 5 6 7
        +-+-+-+-+-+-+-+-+
        |X|R|N|S|R| PID | (REQUIRED)
        +-+-+-+-+-+-+-+-+
    */
    // VP8 payload descriptor
    uint8_t extended_control_bits = ptr[0] & 0x80;  //X
    uint8_t start_of_vp8_partition = ptr[0] & 0x10; //S
    ptr++;
        /*
         0 1 2 3 4 5 6 7
        +-+-+-+-+-+-+-+-+
        |X|R|N|S|R| PID | (REQUIRED)
        +-+-+-+-+-+-+-+-+
    X:  |I|L|T|K|   RSV | (OPTIONAL)
        +-+-+-+-+-+-+-+-+
    I:  |M|  PictureID  | (OPTIONAL)
        +-+-+-+-+-+-+-+-+
        |   PictureID   |
        +-+-+-+-+-+-+-+-+
    L:  |   TL0PICIDX   | (OPTIONAL)
        +-+-+-+-+-+-+-+-+
    T/K:|TID|Y|  KEYIDX | (OPTIONAL)
        +-+-+-+-+-+-+-+-+
    */
        //如果存在X extension
    if (extended_control_bits && ptr < pend) {
        uint8_t pictureid_present;
        uint8_t tl0picidx_present;
        uint8_t tid_present;
        uint8_t keyidx_present;
                //I OPTIONAL
        pictureid_present = ptr[0] & 0x80;
        //L OPTIONAL
        tl0picidx_present = ptr[0] & 0x40;
        //T OPTIONAL
        tid_present = ptr[0] & 0x20;
        //K OPTIONAL
        keyidx_present = ptr[0] & 0x10;
        ptr++;
                //如果I OPTIONAL存在
        if (pictureid_present && ptr < pend) {
            uint16_t picture_id;
                        //获取PictureID
            picture_id = ptr[0] & 0x7F;
            if ((ptr[0] & 0x80) && ptr + 1 < pend) {
                picture_id = (picture_id << 8) | ptr[1];
                ptr++;
            }
            ptr++;
        }
        //如果L OPTIONAL存在
        if (tl0picidx_present && ptr < pend) {
            // ignore temporal level zero index
            ptr++;
        }
        //如果T OPTIONAL或者K OPTIONAL存在
        if ((tid_present || keyidx_present) && ptr < pend) {
            // ignore KEYIDX
            ptr++;
        }
    }
        // VP8 payload header (3 octets)
    // 如果是新的一个vp8帧开始，查看vp8 payload header,
     // 看看是否为keyframe
    if (start_of_vp8_partition) {
        //当新的一个vp8帧开始
        /*
        0 1 2 3 4 5 6 7
        +-+-+-+-+-+-+-+-+
        |Size0|H| VER |P|
        +-+-+-+-+-+-+-+-+
        | Size1 |
        +-+-+-+-+-+-+-+-+
        | Size2 |
        +-+-+-+-+-+-+-+-+
        */
        // P: 关键帧逆使能，0位关键帧，1为非关键帧
        if ((ptr[0] & 0x01) == 0) {// PID == 0 mean keyframe
            return true;
        }
    }

    return false;
}

void RtpDecodeVp8::decode(const RtpPacket::Ptr& rtp)
{
    auto payload_size = rtp->getPayloadSize();
    if (payload_size <= 0) {
        // 无实际负载  [AUTO-TRANSLATED:305af48f]
        // No actual payload
        return ;
    }
    auto ptr = rtp->getPayload();
    auto stamp = rtp->getStampMS();
    auto seq = rtp->getSeq();
    auto pend = ptr + payload_size;
    /*
         0 1 2 3 4 5 6 7
        +-+-+-+-+-+-+-+-+
        |X|R|N|S|R| PID | (REQUIRED)
        +-+-+-+-+-+-+-+-+
    */
    // VP8 payload descriptor
    uint8_t extended_control_bits = ptr[0] & 0x80;  //X
    uint8_t start_of_vp8_partition = ptr[0] & 0x10; //S
    ptr++;
        /*
         0 1 2 3 4 5 6 7
        +-+-+-+-+-+-+-+-+
        |X|R|N|S|R| PID | (REQUIRED)
        +-+-+-+-+-+-+-+-+
    X:  |I|L|T|K|   RSV | (OPTIONAL)
        +-+-+-+-+-+-+-+-+
    I:  |M|  PictureID  | (OPTIONAL)
        +-+-+-+-+-+-+-+-+
        |   PictureID   |
        +-+-+-+-+-+-+-+-+
    L:  |   TL0PICIDX   | (OPTIONAL)
        +-+-+-+-+-+-+-+-+
    T/K:|TID|Y|  KEYIDX | (OPTIONAL)
        +-+-+-+-+-+-+-+-+
    */
        //如果存在X extension
    if (extended_control_bits && ptr < pend) {
        uint8_t pictureid_present;
        uint8_t tl0picidx_present;
        uint8_t tid_present;
        uint8_t keyidx_present;
                //I OPTIONAL
        pictureid_present = ptr[0] & 0x80;
        //L OPTIONAL
        tl0picidx_present = ptr[0] & 0x40;
        //T OPTIONAL
        tid_present = ptr[0] & 0x20;
        //K OPTIONAL
        keyidx_present = ptr[0] & 0x10;
        ptr++;
                //如果I OPTIONAL存在
        if (pictureid_present && ptr < pend) {
            uint16_t picture_id;
                        //获取PictureID
            picture_id = ptr[0] & 0x7F;
            if ((ptr[0] & 0x80) && ptr + 1 < pend) {
                picture_id = (picture_id << 8) | ptr[1];
                ptr++;
            }
            ptr++;
        }
        //如果L OPTIONAL存在
        if (tl0picidx_present && ptr < pend) {
            // ignore temporal level zero index
            ptr++;
        }
        //如果T OPTIONAL或者K OPTIONAL存在
        if ((tid_present || keyidx_present) && ptr < pend) {
            // ignore KEYIDX
            ptr++;
        }
    }
        // VP8 payload header (3 octets)
    // 如果是新的一个vp8帧开始，查看vp8 payload header,
     // 看看是否为keyframe
    if (start_of_vp8_partition) {
        //当新的一个vp8帧开始
        /*
        0 1 2 3 4 5 6 7
        +-+-+-+-+-+-+-+-+
        |Size0|H| VER |P|
        +-+-+-+-+-+-+-+-+
        | Size1 |
        +-+-+-+-+-+-+-+-+
        | Size2 |
        +-+-+-+-+-+-+-+-+
        */
        // P: 关键帧逆使能，0位关键帧，1为非关键帧
        if ((ptr[0] & 0x01) == 0) {// PID == 0 mean keyframe
            logInfo << ("vp8 get keyframe");
            _frame->_isKeyframe = true;
        }
                //当新vp8帧来了，上送上一个vp8帧给业务
        // outputFrame(rtp, _frame);
        // _frame->_buffer.assign("\x00\x00\x00\x01", 4);
        _frame->_pts = stamp;
    }
    
    _frame->_buffer.append((char *) ptr, pend - ptr);

    if (rtp->getHeader()->mark) {
        onFrame(_frame);
    }
}

void RtpDecodeVp8::setOnDecode(const function<void(const FrameBuffer::Ptr& frame)> cb)
{
    _onFrame = cb;
}

void RtpDecodeVp8::onFrame(const FrameBuffer::Ptr& frame)
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