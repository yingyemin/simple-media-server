#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Frame.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

unordered_map<string, FrameBuffer::funcCreateFrame> FrameBuffer::_mapCreateFrame;

FrameBuffer::FrameBuffer()
{

}

const char* FrameBuffer::findNextNalu(const char* p, size_t bytes, size_t& leading)
{
    size_t i, zeros;
    for (zeros = i = 0; i + 1 < bytes; i++)
    {
        if (0x01 == p[i] && zeros >= 2)
        {
            assert(i >= zeros);
            leading = (zeros + 1) > 4 ? 4 : (zeros + 1); // zeros + 0x01
            return p +i + 1 - leading;
        }

        zeros = 0x00 != p[i] ? 0 : (zeros + 1);
    }

    return nullptr;
}

int FrameBuffer::startSize(const char* data, int len)
{
    if (len < 4) {
        return 0;
    }

    if (*(uint16_t*)data != 0) {
        return 0;
    }

    if (data[2] == 1) {
        return 3;
    }

    if (*(uint16_t*)(data + 2) == 0x1000) {
        return 4;
    }

    return 0;
}

FrameBuffer::Ptr FrameBuffer::createFrame(const string& codecName, int startSize, int index, bool addStart)
{
    auto iter = _mapCreateFrame.find(codecName);
    if (iter != _mapCreateFrame.end()) {
        return iter->second(startSize, index, addStart);
    } else {
        auto frame = make_shared<FrameBuffer>();
        
        frame->_startSize = startSize;
        frame->_codec = codecName;
        frame->_index = index;
        frame->_trackType = 1;//AudioTrackType;

        return frame;
    }
}

void FrameBuffer::registerFrame(const string& codecName, const funcCreateFrame& func)
{
    _mapCreateFrame[codecName] = func;
}