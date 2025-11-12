#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Ehome5Parser.h"
#include "Logger.h"
#include "Util/String.hpp"

using namespace std;

void Ehome5Parser::parse(const char *data, size_t len)
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
        int len = end - data;
        if (len < _offset) {
            break;
        }

        uint16_t payloadSize = (((uint8_t *)data)[_offset - 2] << 8) | ((uint8_t *)data)[_offset - 1];
        // logInfo << "payloadSize: " << payloadSize << ", len: " << len << ", used : " << (data - start);
        if (len >= payloadSize + _offset) {
            onEhomePacket(data, payloadSize + _offset);
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

void Ehome5Parser::setOnEhomePacket(const function<void(const char* data, int len)>& cb)
{
    _onEhomePacket = cb;
}

void Ehome5Parser::onEhomePacket(const char* data, int len)
{
    // logInfo << "on rtp packet";
    if (_onEhomePacket) {
        _onEhomePacket(data, len);
    }
}
