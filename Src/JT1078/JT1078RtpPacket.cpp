#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "JT1078RtpPacket.h"
#include "Logger.h"
#include "Util/String.h"
#include "Codec/H264Frame.h"
#include "Codec/H265Frame.h"

using namespace std;

JT1078RtpPacket::JT1078RtpPacket(const StreamBuffer::Ptr& buffer)
{
    _buffer = buffer;
    _header = (JT1078RtpHeader*)(data());
}

char* JT1078RtpPacket::getPayload()
{
    return _buffer->data() + _payloadIndex;
}

size_t JT1078RtpPacket::getPayloadSize()
{
    return _buffer->size() - _payloadIndex;
}

char* JT1078RtpPacket::data()
{
    return _buffer->data();
}

size_t JT1078RtpPacket::size()
{
    return _buffer->size();
}

StreamBuffer::Ptr JT1078RtpPacket::buffer()
{
    return _buffer;
}

string JT1078RtpPacket::getCodecType()
{
    switch (_header->pt) {
    case 6:
        _codec = "g711a";
        break;
    case 7:
        _codec = "g711u";
        break;
    case 19:
        _codec = "aac";
        break;
    case 98:
        _codec = "h264";
        break;
    case 99:
        _codec = "h265";
        break;
    default:
        // logInfo << "invalid payload type: " << (int)_header->pt;
        break;
    }

    return _codec;
}

JT1078_STREAM_TYPE JT1078RtpPacket::getStreamType()
{
    switch (_header->dataType)
    {
    case 0x00://I
        _streamType = JT1078_VideoI;
        _type = "video";
        break;
    case 0x01://P
        _streamType = JT1078_VideoP;
        _type = "video";
        break;
    case 0x02://B
        _streamType = JT1078_VideoB;
        _type = "video";
        break;
    case 0x03://音频
        _streamType = JT1078_Audio;
        _type = "audio";
        break;
    case 0x04://透传
        _streamType = JT1078_Passthrough;
        break;
    default:
        printf("不支持的流类型\n");
        break;
    }

    return _streamType;
}

string JT1078RtpPacket::getTrackType()
{
    if (!_type.empty()) {
        return _type;
    }

    getStreamType();
    return _type;
}

uint16_t JT1078RtpPacket::getSeq()
{
    return ntohs(_header->seq);
}

JT1078_SUBMARK JT1078RtpPacket::getSubMark()
{
    switch (_header->subPackageHandleMark)
    {
    case 0x00:
        _subMark = JT1078_Atomic; 
        break;
    case 0x01:
        _subMark = JT1078_First;
        break;
    case 0x02:
        _subMark = JT1078_Last; 
        break;
    case 0x03:
        _subMark = JT1078_Intermediate; 
        break;
    default:
        printf("不支持的分包处理标识\n");
    }

    return _subMark;
}

string JT1078RtpPacket::getSimCode()
{
    string simCode;
    size_t pos = 0;
    uint8_t tmp = _header->simNo[pos];
    tmp = (tmp >> 4) * 10 + (tmp & 0x0f);
    
    if (tmp / 10 == 0) {
        simCode.push_back(tmp % 10 + '0');
        ++pos;
    }
    for (; pos < 6; ++pos) {
        tmp = _header->simNo[pos];
        tmp = (tmp >> 4) * 10 + (tmp & 0x0f);

        simCode.push_back(tmp / 10 + '0');
        simCode.push_back(tmp % 10 + '0');
    }

    return simCode;
}

uint64_t JT1078RtpPacket::getTimestamp()
{
    if (getStreamType() < 4) {
        unsigned char* payload = (unsigned char*)_buffer->data() + 16;
        uint64_t stamp = 0;
        for (uint8_t i = 0; i < 8; i++)
        {
            stamp <<= 8;
            stamp |= payload[i];
        }
        return stamp;
    }

    return 0;
}

bool JT1078RtpPacket::isStartGop()
{
    string codec = getCodecType();
    if (codec == "h264" || codec == "h265") {
        auto payload = getPayload();
        auto payloadSize = getPayloadSize();
        if (payloadSize < 5) {
            return false;
        }

        if (payload[0] == 0x00 && payload[1] == 0x00 && payload[2] == 0x00 && payload[3] == 0x01) {
            if (codec == "h264") {
                if (H264Frame::getNalType(payload[4]) == H264NalType::H264_SPS) {
                    return true;
                }
            } else {
                if (H265Frame::getNalType(payload[4]) == H265NalType::H265_VPS) {
                    return true;
                }
            }
        }
    }

    return false;
}