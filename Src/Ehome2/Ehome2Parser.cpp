#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Ehome2Parser.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

static const int  EHOME_HEADER_SIZE = 256;

void Ehome2Parser::parse(const char *data, size_t len)
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

    
    if (_first) {
        if (len < EHOME_HEADER_SIZE) {
            return ;
        }
        if (data[0] == 0x01 && data[1] == 0x00 && data[2] >= 0x01) {
            data += EHOME_HEADER_SIZE;
            len -= EHOME_HEADER_SIZE;
        }
        _first = false;
        _offset = 4;
    }

    auto start = data;
    auto end = data + len;
    
    while (data < end) {
        int len = end - data;
        if (len < _offset) {
            break;
        }

        uint16_t payloadSize = (((uint8_t *)data)[_offset - 2] << 8) | ((uint8_t *)data)[_offset - 1];
        // logInfo << "payloadSize: " << payloadSize << ", len: " << len << ", used : " << (data - start);
        if (len >= payloadSize + _offset) {
            int moveOffset = 1;
            // 0x0d atomic packet
            uint8_t flag = data[17];
            if (data[16] == 0x1c && 
                (flag == 0x80/*first packet*/ || flag == 0x00/*middle*/ || flag == 0x40/*last*/)) {
                moveOffset = 2;
            }
            memmove((char *) data + moveOffset, data, 16);
            data += moveOffset;
            payloadSize -= moveOffset;
            onRtpPacket(data, payloadSize + _offset);
            // static int i = 0;
            // string name = "test" + to_string(i++);
            // FILE* fp = fopen(name.data(), "wb");
            // fwrite(data, 1, payloadSize + 2, fp);
            // fclose(fp);
            data += payloadSize + _offset;
        } else {
            break;
        }
    }

    if (data < end) {
        // logInfo << "have remain data: " << (end - data);
        _remainData.assign(data, end - data);
    } else {
        // logInfo << "don't have remain data";
        _remainData.clear();
    }
}

void Ehome2Parser::setOnRtpPacket(const function<void(const char* data, int len)>& cb)
{
    _onRtpPacket = cb;
}

void Ehome2Parser::onRtpPacket(const char* data, int len)
{
    // logInfo << "on rtp packet";
    if (_onRtpPacket) {
        _onRtpPacket(data, len);
    }
}
