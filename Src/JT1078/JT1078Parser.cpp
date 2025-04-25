#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "JT1078Parser.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

void JT1078Parser::parse(const char *data, size_t len)
{
    if (_version == JT1078_2019) {
        parse2019(data, len);
    } else {
        parse2016(data, len);
    }
}

void JT1078Parser::parse2016(const char *data, size_t len)
{
    // 从配置中获取
    static int maxRemainSize = 4 * 1024 * 1024;

    int remainSize = _remainData.size();
    if (remainSize > maxRemainSize) {
        logError << "remain cache is too large";
        _remainData.clear();
        return ;
    }

    if (remainSize > 0) {
        _remainData.append(data, len);
        data = _remainData.data();
        len += remainSize;
    }

    auto start = data;
    auto end = data + len;
    
    while (data < end) {
        bool findHeader = false;
        while (true) {
            if (end - data < 4) {
                break;
            }
            if ((data[0] == 0x30) && (data[1] == 0x31) && 
                    (data[2] == 0x63) && (data[3] == 0x64)) {
                findHeader = true;
                break;
            } else {
                ++data;
            }
        }

        if (!findHeader) {
            break;
        }

        if (end - data < sizeof(JT1078RtpHeader)) {
            break;
        }

        JT1078RtpHeader* header = (JT1078RtpHeader*)data;

        int index = 0;
        int payloadIndex = 0;
        int length = 0;
        switch (header->dataType)
        {
        case 0x00://I
        case 0x01://P
        case 0x02://B
            payloadIndex = 30;
            if (end - data < 30) {
                break;
            }
            length = readUint16BE((char*)data + 28);
            if (end - data < 30 + length) {
                break;
            }
            index = 30 + length;
            break;  
        case 0x03://音频
            payloadIndex = 26;
            if (end - data < 26) {
                break;
            }
            length = readUint16BE((char*)data + 24);
            if (end - data < 26 + length) {
                break;
            }
            index = 26 + length;
            break;
        case 0x04://透传
            payloadIndex = 18;
            if (end - data < 18) {
                break;
            }
            length = readUint16BE((char*)data + 16);
            if (end - data < 18 + length) {
                break;
            }
            index = 18 + length;
            break;
        default:
            printf("不支持的流类型\n");
            index = -1;
            break;
        }

        if (index == 0) {
            break;
        } else if (index == -1) {
            data += 4;
            continue;
        }

        if (_version == JT1078_0) {
            if (end - data < index + 4) {
                break;
            }

            if ((data[index] == 0x30) && (data[index + 1] == 0x31) && 
                    (data[index + 2] == 0x63) && (data[index + 3] == 0x64)) {
                _version = JT1078_2016;
            } else {
                _version = JT1078_2019;
                return ;
            }
        }

        // if (length <= 950) {
            // 协议规定，包不能大于950,但是有些设备不按照标准来，先去掉这个限制
            StreamBuffer::Ptr buffer = StreamBuffer::create();
            if (length % 80 == 4 && data[payloadIndex] == 0x00 && data[payloadIndex + 1] == 0x01 && 
                (uint8_t)data[payloadIndex + 2] == 0xa0 && data[payloadIndex + 3] == 0x00) {
                payloadIndex += 4;
            }
            buffer->assign(data, index);
            auto rtp = make_shared<JT1078RtpPacket>(buffer, JT1078_2016);
            rtp->setPayloadIndex(payloadIndex);
            onRtpPacket(rtp);
        // }

        data += index;
    }

    if (data < end) {
        // logInfo << "have remain data: " << (end - data);
        _remainData.assign(data, end - data);
    } else {
        // logInfo << "don't have remain data";
        _remainData.clear();
    }
}

void JT1078Parser::parse2019(const char *data, size_t len)
{
    // 从配置中获取
    static int maxRemainSize = 4 * 1024 * 1024;

    int remainSize = _remainData.size();
    if (remainSize > maxRemainSize) {
        logError << "remain cache is too large";
        _remainData.clear();
        return ;
    }

    if (remainSize > 0) {
        _remainData.append(data, len);
        data = _remainData.data();
        len += remainSize;
    }

    auto start = data;
    auto end = data + len;
    
    while (data < end) {
        bool findHeader = false;
        while (true) {
            if (end - data < 4) {
                break;
            }
            if ((data[0] == 0x30) && (data[1] == 0x31) && 
                    (data[2] == 0x63) && (data[3] == 0x64)) {
                findHeader = true;
                break;
            } else {
                ++data;
            }
        }

        if (!findHeader) {
            break;
        }

        if (end - data < sizeof(JT1078RtpHeader2019)) {
            break;
        }

        JT1078RtpHeader2019* header = (JT1078RtpHeader2019*)data;

        int index = 0;
        int payloadIndex = 0;
        int length = 0;
        switch (header->dataType)
        {
        case 0x00://I
        case 0x01://P
        case 0x02://B
            payloadIndex = 34;
            if (end - data < 34) {
                break;
            }
            length = readUint16BE((char*)data + 32);
            if (end - data < 34 + length) {
                break;
            }
            index = 34 + length;
            break;  
        case 0x03://音频
            payloadIndex = 30;
            if (end - data < 30) {
                break;
            }
            length = readUint16BE((char*)data + 28);
            if (end - data < 30 + length) {
                break;
            }
            index = 30 + length;
            break;
        case 0x04://透传
            payloadIndex = 22;
            if (end - data < 22) {
                break;
            }
            length = readUint16BE((char*)data + 20);
            if (end - data < 22 + length) {
                break;
            }
            index = 22 + length;
            break;
        default:
            printf("不支持的流类型\n");
            index = -1;
            break;
        }

        if (index == 0) {
            break;
        } else if (index == -1) {
            data += 4;
            continue;
        }

        // if (length <= 950) {
            // 协议规定，包不能大于950,但是有些设备不按照标准来，先去掉这个限制
            StreamBuffer::Ptr buffer = StreamBuffer::create();
            if (length % 80 == 4 && data[payloadIndex] == 0x00 && data[payloadIndex + 1] == 0x01 && 
                (uint8_t)data[payloadIndex + 2] == 0xa0 && data[payloadIndex + 3] == 0x00) {
                payloadIndex += 4;
            }
            buffer->assign(data, index);
            auto rtp = make_shared<JT1078RtpPacket>(buffer, JT1078_2019);
            rtp->setPayloadIndex(payloadIndex);
            onRtpPacket(rtp);
        // }

        data += index;
    }

    if (data < end) {
        // logInfo << "have remain data: " << (end - data);
        _remainData.assign(data, end - data);
    } else {
        // logInfo << "don't have remain data";
        _remainData.clear();
    }
}

void JT1078Parser::setOnRtpPacket(const function<void(const JT1078RtpPacket::Ptr& buffer)>& cb)
{
    _onRtpPacket = cb;
}

void JT1078Parser::onRtpPacket(const JT1078RtpPacket::Ptr& buffer)
{
    // logInfo << "on rtp packet";
    if (_onRtpPacket) {
        _onRtpPacket(buffer);
    }
}
