#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "AacFrame.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Config.h"

using namespace std;

static FrameBuffer::Ptr getAdtsAac()
{
    auto frame = make_shared<FrameBuffer>();
    const int profile = 1;    //AAC LC
    const int frequency_index = 0xb;  //8KHz
    const int channel_configuration = 1;  //MPEG-4 Audio Channel Configuration.
    unsigned int packetLen = 6;
    frame->_buffer.resize(13, 0x00);
    int m_data_len = 0;
    frame->_buffer[m_data_len++] = (char)0xFF;
    frame->_buffer[m_data_len++] = (char)0xF1; 
    frame->_buffer[m_data_len++] = (char)(((profile - 1) << 6) + (frequency_index << 2) + (channel_configuration >> 2));
    frame->_buffer[m_data_len++] = (char)((channel_configuration & 0x3) << 6 | (packetLen >> 11));
    frame->_buffer[m_data_len++] = (char)((packetLen & 0x7FF) >> 3);
    frame->_buffer[m_data_len++] = (char)(((packetLen & 7) << 5) + 0x1F);
    frame->_buffer[m_data_len++] = (char)0xFC;
    frame->_buffer[m_data_len++] = (char)0x21;
    frame->_buffer[m_data_len++] = (char)0x10;
    frame->_buffer[m_data_len++] = (char)0x04;
    frame->_buffer[m_data_len++] = (char)0x60;
    frame->_buffer[m_data_len++] = (char)0x8C;
    frame->_buffer[m_data_len++] = (char)0x1C;

    frame->_codec = "aac";
    frame->_trackType = 1; //AudioTrackType
    frame->_startSize = 7;
    frame->_index = 1;

    return frame;
}

static StreamBuffer::Ptr getFlvAac()
{
    auto frame = make_shared<StreamBuffer>(2 + 6 + 1);
    auto data = frame->data();

    int audioType = 10;
	int flvSampleRate = 0;
	uint8_t flvStereoOrMono = 0;
	uint8_t flvSampleBit = 0;

	int audoFlag = (audioType << 4) | (flvSampleRate << 2) | (flvSampleBit << 1) | flvStereoOrMono;

    data[0] = audoFlag;
    data[1] = 1;

    int m_data_len = 2;
    data[m_data_len++] = (char)0x21;
    data[m_data_len++] = (char)0x10;
    data[m_data_len++] = (char)0x04;
    data[m_data_len++] = (char)0x60;
    data[m_data_len++] = (char)0x8C;
    data[m_data_len++] = (char)0x1C;

    return frame;
}

void AacFrame::split(const function<void(const FrameBuffer::Ptr& frame)>& cb)
{
    auto payload = (uint8_t*)_buffer.data();
    uint32_t size = _buffer.size();

    while (size >= 7) { 
        if (!(payload[0] == 0xFF && (payload[1] & 0xF0) == 0xF0)) {
            return ;
        }

        uint32_t aacLen = (payload[3] & 0x03 << 11) | payload[4] << 3 | payload[5] >> 5;
        if (aacLen > size) {
            return ;
        }
        auto subframe = make_shared<AacFrame>();
        subframe->_startSize = 7;
        subframe->_buffer.assign((char*)payload, aacLen);
        subframe->_pts = _pts; // pts * 1000 / 90000,计算为毫秒
        subframe->_dts = _dts;
        subframe->_index = _index;
        subframe->_trackType = _trackType;
        subframe->_codec = _codec;

        cb(subframe);

        payload += aacLen;
        size -= aacLen;
    }
}

FrameBuffer::Ptr AacFrame::getMuteForAdts()
{
    static FrameBuffer::Ptr adtsAac = getAdtsAac();
    return adtsAac;
}

StreamBuffer::Ptr AacFrame::getMuteForFlv()
{
    static StreamBuffer::Ptr flvAac = getFlvAac();
    return flvAac;
}

FrameBuffer::Ptr AacFrame::createFrame(int startSize, int index, bool addStart)
{
    auto frame = make_shared<AacFrame>();
        
    frame->_startSize = startSize;
    frame->_codec = "aac";
    frame->_index = index;
    frame->_trackType = 1;//AudioTrackType;

    return frame;
}

void AacFrame::registerFrame()
{
	FrameBuffer::registerFrame("aac", AacFrame::createFrame);
}