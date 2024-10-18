#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "GB28181Parser.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

void GB28181Parser::parse(const char *data, size_t len)
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
            len -= 2;
            data += 2;
            _rtpBuffer = make_shared<StreamBuffer>(_contentLen + 1);
            _rtpBuffer->setSize(0);
            _stage = 2;
        }
        // logInfo << "_contentLen: " << _contentLen << ", len: " << len << ", used : " << (data - start);
        if (len >= _contentLen) {
            auto payload = _rtpBuffer->data() + _rtpBuffer->size();
            memcpy(payload, data, _contentLen);
            _rtpBuffer->setSize(_contentLen + _rtpBuffer->size());
            onRtpPacket(_rtpBuffer);
            // static int i = 0;
            // string name = "test" + to_string(i++);
            // FILE* fp = fopen(name.data(), "wb");
            // fwrite(data, 1, payloadSize + 2, fp);
            // fclose(fp);
            data += _contentLen;
            _contentLen = 0;
            _stage = 1;
        } else {
            auto payload = _rtpBuffer->data() + _rtpBuffer->size();
            memcpy(payload, data, len);
            _rtpBuffer->setSize(len + _rtpBuffer->size());
            _contentLen -= len;
            data += len;
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

void GB28181Parser::setOnRtpPacket(const function<void(const StreamBuffer::Ptr& buffer)>& cb)
{
    _onRtpPacket = cb;
}

void GB28181Parser::onRtpPacket(const StreamBuffer::Ptr& buffer)
{
    // logInfo << "on rtp packet";
    if (_onRtpPacket) {
        _onRtpPacket(buffer);
    }
}
