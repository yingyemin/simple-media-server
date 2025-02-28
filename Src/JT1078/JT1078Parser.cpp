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

        // if (length <= 950) {
            // 协议规定，包不能大于950,但是有些设备不按照标准来，先去掉这个限制
            StreamBuffer::Ptr buffer = StreamBuffer::create();
            buffer->assign(data, index);
            auto rtp = make_shared<JT1078RtpPacket>(buffer);
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
