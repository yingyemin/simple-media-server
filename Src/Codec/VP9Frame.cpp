#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "VP9Frame.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Config.h"

using namespace std;

VP9Frame::VP9Frame(const VP9Frame::Ptr& frame)
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

void VP9Frame::split(const function<void(const FrameBuffer::Ptr& frame)>& cb)
{
    if (!cb) {
        return ;
    }

    cb(shared_from_this());
}

bool VP9Frame::isNewNalu()
{
    return true;
}

FrameBuffer::Ptr VP9Frame::createFrame(int startSize, int index, bool addStart)
{
    auto frame = make_shared<VP9Frame>();
        
    frame->_startSize = startSize;
    frame->_codec = "vp9";
    frame->_index = index;
    frame->_trackType = 0;//VideoTrackType;

    // if (addStart) {
    //     frame->_buffer.assign("\x00\x00\x00\x01", 4);
    // };

    return frame;
}

void VP9Frame::registerFrame()
{
	FrameBuffer::registerFrame("vp9", VP9Frame::createFrame);
}

bool VP9Frame::keyFrame() const
{
    return getNalType((uint8_t*)data(), size());
}

uint8_t VP9Frame::getNalType(uint8_t* nalByte, int len)
{
    const static uint8_t startcode[] = { 0x49, 0x83, 0x42 };

    if (len < 4 || nalByte[1] != startcode[0] || nalByte[2] != startcode[1] || nalByte[3] != startcode[2]) {
        return false;
    }

    VP9Bitstream bitsream(nalByte, len);
    bitsream.GetWord(2); // frame marker
    int profile = bitsream.GetBit() | bitsream.GetBit() << 1;
    if (profile == 3) {
        bitsream.GetBit(); //shall be 0
    }

    int show_existing_frame = bitsream.GetBit();
    if (show_existing_frame) {
        bitsream.GetWord(3); // frame_to_show_map_idx
    }

    int frame_type = bitsream.GetBit();

    return frame_type == VP9_KEYFRAME;
}