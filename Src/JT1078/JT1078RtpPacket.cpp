#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#ifndef _WIN32
#include <arpa/inet.h>
#endif

#include "JT1078RtpPacket.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "Codec/H264Frame.h"
#include "Codec/H265Frame.h"

using namespace std;

JT1078RtpPacket::JT1078RtpPacket(const StreamBuffer::Ptr& buffer, JT1078_VERSION version)
    :_version(version)
    ,_buffer(buffer)
{
    // _buffer = buffer;
    if (version == JT1078_2016) {
        _header = (JT1078RtpHeader*)(data());
    } else {
        _header2019 = (JT1078RtpHeader2019*)(data());
    }
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

uint8_t JT1078RtpPacket::getPt() 
{
    if (_version == JT1078_2016) {
        return _header->pt;
    } else {
        return _header2019->pt;
    }
}

string JT1078RtpPacket::getCodecType()
{
    switch (getPt()) {
    case 6:
        _codec = "g711a";
        break;
    case 7:
        _codec = "g711u";
        break;
    case 8:
        _codec = "g726";
        break;
    case 19:
        _codec = "aac";
        break;
    case 26:
        _codec = "adpcma";
        break;
    case 98:
        _codec = "h264";
        break;
    case 99:
        _codec = "h265";
        break;
    default:
        logTrace << "invalid payload type: " << (int)getPt();
        break;
    }

    return _codec;
}

uint8_t JT1078RtpPacket::getDataType()
{
    if (_version == JT1078_2016) {
        return _header->dataType;
    } else {
        return _header2019->dataType;
    }
}

JT1078_STREAM_TYPE JT1078RtpPacket::getStreamType()
{
    switch (getDataType())
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
        logTrace << "不支持的流类型: " << (int)getDataType();
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

uint16_t JT1078RtpPacket::getHeaderSeq()
{
    if (_version == JT1078_2016) {
        return _header->seq;
    } else {
        return _header2019->seq;
    }
}

int JT1078RtpPacket::getLogicNo()
{
    if (_version == JT1078_2016) {
        return _header->logicChannelNumber;
    } else {
        return _header2019->logicChannelNumber;
    }
}

uint8_t JT1078RtpPacket::getHeaderSubMark()
{
    if (_version == JT1078_2016) {
        return _header->subPackageHandleMark;
    } else {
        return _header2019->subPackageHandleMark;
    }
}

bool JT1078RtpPacket::getMark()
{
    if (_version == JT1078_2016) {
        return _header->mark;
    } else {
        return _header2019->mark;
    }
}

uint16_t JT1078RtpPacket::getSeq()
{
    return ntohs(getHeaderSeq());
}

JT1078_SUBMARK JT1078RtpPacket::getSubMark()
{
    switch (getHeaderSubMark())
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
        logTrace << "不支持的分包处理标识" << (int)getHeaderSubMark();
    }

    return _subMark;
}

// string JT1078RtpPacket::getSimCode()
// {
//     string simCode;
//     size_t pos = 0;
//     uint8_t tmp = _header->simNo[pos];
//     tmp = (tmp >> 4) * 10 + (tmp & 0x0f);
    
//     if (tmp / 10 == 0) {
//         simCode.push_back(tmp % 10 + '0');
//         ++pos;
//     }
//     for (; pos < 6; ++pos) {
//         tmp = _header->simNo[pos];
//         tmp = (tmp >> 4) * 10 + (tmp & 0x0f);

//         simCode.push_back(tmp / 10 + '0');
//         simCode.push_back(tmp % 10 + '0');
//     }

//     return simCode;
// }

string JT1078RtpPacket::getSimCode()
{
    string simCode;
    uint8_t tmp;
    if (_version == JT1078_2016) {
        for (size_t pos = 0; pos < 6; ++pos) {
            tmp = _header->simNo[pos];
            tmp = (tmp >> 4) * 10 + (tmp & 0x0f);

            simCode.push_back(tmp / 10 + '0');
            simCode.push_back(tmp % 10 + '0');
        }
    } else {
        for (size_t pos = 0; pos < 10; ++pos) {
            tmp = _header2019->simNo[pos];
            tmp = (tmp >> 4) * 10 + (tmp & 0x0f);

            simCode.push_back(tmp / 10 + '0');
            simCode.push_back(tmp % 10 + '0');
        }
    }

    return simCode;
}

uint64_t JT1078RtpPacket::getTimestamp()
{
    if (getStreamType() < 4) {
        int offset = 16;
        if (_version == JT1078_2019) {
            offset = 20;
        }
        unsigned char* payload = (unsigned char*)_buffer->data() + offset;
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
