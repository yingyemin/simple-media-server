#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "VP8Frame.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "Common/Config.h"

using namespace std;

VP8Frame::VP8Frame(const VP8Frame::Ptr& frame)
{
    if (frame) {
        _codec = frame->_codec;
        _trackType = frame->_trackType;
        _pts = frame->_pts;
        _profile = frame->_profile;
        _index = frame->_index;
        _dts = frame->_dts;
    }
}

void VP8Frame::split(const function<void(const FrameBuffer::Ptr& frame)>& cb)
{
    if (!cb) {
        return ;
    }

    cb(shared_from_this());
}

bool VP8Frame::isNewNalu()
{
    return true;
}

FrameBuffer::Ptr VP8Frame::createFrame(int startSize, int index, bool addStart)
{
    auto frame = make_shared<VP8Frame>();
        
    frame->_startSize = startSize;
    frame->_codec = "vp8";
    frame->_index = index;
    frame->_trackType = 0;//VideoTrackType;

    // if (addStart) {
    //     frame->_buffer.assign("\x00\x00\x00\x01", 4);
    // };

    return frame;
}

void VP8Frame::registerFrame()
{
	FrameBuffer::registerFrame("vp8", VP8Frame::createFrame);
}

bool VP8Frame::keyFrame() const
{
    uint32_t tag;
    const uint8_t* p;
    const static uint8_t startcode[] = { 0x9d, 0x01, 0x2a };

    if (size() < 10)
        return false;

    p = (const uint8_t*)data();

    // 9.1.  Uncompressed Data Chunk
    tag = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
    //key_frame = tag & 0x01;
    //version = (tag >> 1) & 0x07;
    //show_frame = (tag >> 4) & 0x1;
    //first_part_size = (tag >> 5) & 0x7FFFF;

    if (0 != (tag & 0x01) || startcode[0] != p[3] || startcode[1] != p[4] || startcode[2] != p[5]) {
        return false; // not key frame
    } else {
        return true;
    }
}

uint8_t VP8Frame::getNalType(uint8_t* nalByte, int len)
{
    uint32_t tag;
    const uint8_t* p;
    const static uint8_t startcode[] = { 0x9d, 0x01, 0x2a };

    if (len < 10)
        return -1;

    p = (const uint8_t*)nalByte;

    // 9.1.  Uncompressed Data Chunk
    tag = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
    //key_frame = tag & 0x01;
    //version = (tag >> 1) & 0x07;
    //show_frame = (tag >> 4) & 0x1;
    //first_part_size = (tag >> 5) & 0x7FFFF;

    if (0 != (tag & 0x01) || startcode[0] != p[3] || startcode[1] != p[4] || startcode[2] != p[5]) {
        return VP8_FRAME; // not key frame
    } else {
        return VP8_KEYFRAME;
    }
}