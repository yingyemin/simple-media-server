#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "WebsocketContext.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

/**
 *
  0             1                 2               3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-------+-+-------------+-------------------------------+
 |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 | |1|2|3|       |K|             |                               |
 +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 |     Extended payload length continued, if payload len == 127  |
 + - - - - - - - - - - - - - - - +-------------------------------+
 |                               |Masking-key, if MASK set to 1  |
 +-------------------------------+-------------------------------+
 | Masking-key (continued)       |          Payload Data         |
 +-------------------------------- - - - - - - - - - - - - - - - +
 :                     Payload Data continued ...                :
 + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 |                     Payload Data continued ...                |
 +---------------------------------------------------------------+
 *
 */

void WebsocketContext::decode(unsigned char *data, size_t len)
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
        _remainData.append((char*)data, len);
        data = (unsigned char *)_remainData.data();
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

            _otherHeaderSize = 0;
            _maskIndex = 0;

            _frame.finish = (data[0] >> 7) & 1;
            _frame.rsv1 = (data[0] >> 6) & 1;
            _frame.rsv2 = (data[0] >> 5) & 1;
            _frame.rsv3 = (data[0] >> 4) & 1;
            _frame.opcode = (OpcodeType)(data[0] & 0xf);

            _frame.mask = (data[1] >> 7) & 1;
            _frame.payloadLen = data[1] & 0x7f;

            if (_frame.mask) {
                _otherHeaderSize += 4;
            }

            if (_frame.payloadLen == 126) {
                _otherHeaderSize += 2;
            } else if (_frame.payloadLen == 127) {
                _otherHeaderSize += 8;
            }

            len -= 2;
            data += 2;
            _stage = 2;
        }

        // logInfo << "payloadSize: " << _frame.payloadLen << ", len: " << len << ", _otherHeaderSize : " << (_otherHeaderSize);
        if (_stage == 2) {
            len = end - data;
            if (len < _otherHeaderSize) {
                break;
            }

            if (_frame.payloadLen == 126) {
                _frame.payloadLen = (*data << 8) | *(data + 1);
                data += 2;
                len -= 2;
            } else if (_frame.payloadLen == 127) {
                _frame.payloadLen = (uint64_t)readUint32BE((char*)data) << 32 | readUint32BE((char*)data + 4);
                data += 8;
                len -= 8;
            }

            if (_frame.mask) {
                _frame.maskKey.assign((char*)data, 4);
                data += 4;
                len -= 4;
            }

            _stage = 3;
        }

        // logInfo << "payloadSize: " << _frame.payloadLen << ", len: " << len << ", used : " << (data - start);
        if (len >= _frame.payloadLen) {
            if (_frame.mask) {
                for(int i = 0; i < _frame.payloadLen ; ++i){
                    data[i] ^= _frame.maskKey[_maskIndex++ % 4];
                }
                _maskIndex %= 4;
            }
            onWebsocketFrame((char*)data, _frame.payloadLen);
            // static int i = 0;
            // string name = "test" + to_string(i++);
            // FILE* fp = fopen(name.data(), "wb");
            // fwrite(data, 1, payloadSize + 2, fp);
            // fclose(fp);
            data += _frame.payloadLen;
            _stage = 1;
        } else {
            break;
        }
    }

    if (data < end) {
        logInfo << "have remain data: " << (end - data);
        _remainData.assign((char*)data, end - data);
    } else {
        // logInfo << "don't have remain data";
        _remainData.clear();
    }
}

void WebsocketContext::encodeHeader(const WebsocketFrame& frame, StringBuffer::Ptr& header)
{
    // StringBuffer packet;
    uint8_t one = (frame.finish << 7) | (frame.rsv1 << 6) | (frame.rsv2 << 5) 
                    | (frame.rsv3 << 4) | (frame.opcode & 0xf);

    header->push_back(one);

    auto maskFlag = frame.mask && !frame.maskKey.empty();
    one = maskFlag << 7;
    uint64_t len = frame.payloadLen;

    if (len < 126) {
        header->push_back(len | one);
    } else if (len <= 0xffff) {
        header->push_back(one | 126);

        auto len_low = htons(len);
        header->append((char *)&len_low, 2);
    } else {
        header->push_back(one | 127);

        uint32_t len_low = htonl(len >> 32);
        header->append((char *)&len_low, 4);
        
        len_low = htonl(len & 0xFFFFFFFF);
        header->append((char *)&len_low, 4);
    }

    if (maskFlag) {
        header->append(frame.maskKey.data(), 4);
    }
}

void WebsocketContext::encodePayload(const WebsocketFrame& frame, Buffer::Ptr& payload)
{
    int len = payload->size();
    if (len == 0) {
        return ;
    }

    if (frame.mask && !frame.maskKey.empty()) {
        uint8_t *ptr = (uint8_t*)payload->data();
        for(int i = 0; i < len ; ++i,++ptr){
            *(ptr) ^= frame.maskKey[i % 4];
        }
    }
}

void WebsocketContext::setOnWebsocketFrame(const function<void(const char* data, int len)>& cb)
{
    _onWebsocketFrame = cb;
}

void WebsocketContext::onWebsocketFrame(const char* data, int len)
{
    // logInfo << "on rtp packet";
    if (_onWebsocketFrame) {
        _onWebsocketFrame(data, len);
    }
}
