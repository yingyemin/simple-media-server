#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "WebrtcParser.h"
#include "Logger.h"
#include "Util/String.hpp"

using namespace std;

void WebrtcParser::parse(const char *data, size_t len)
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
        len = end - data;
        if (_stage == 1) {
            if (len < 2) {
                break;
            }

            _contentLen = (((uint8_t *)data)[0] << 8) | ((uint8_t *)data)[1];
            _stage = 2;
        }
        // logInfo << "_contentLen: " << _contentLen << ", len: " << len << ", used : " << (data - start);
        if (len >= _contentLen + 2) {
            onRtcPacket(data, _contentLen + 2);
            // static int i = 0;
            // string name = "test" + to_string(i++);
            // FILE* fp = fopen(name.data(), "wb");
            // fwrite(data, 1, payloadSize + 2, fp);
            // fclose(fp);
            data += _contentLen + 2;
            _stage = 1;
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

void WebrtcParser::setOnRtcPacket(const function<void(const char* data, int len)>& cb)
{
    _onRtcPacket = cb;
}

void WebrtcParser::onRtcPacket(const char* data, int len)
{
    // logInfo << "on rtp packet";
    if (_onRtcPacket) {
        _onRtcPacket(data, len);
    }
}
