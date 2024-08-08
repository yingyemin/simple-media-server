#ifndef WebsocketContext_H
#define WebsocketContext_H

#include "Buffer.h"
#include <unordered_map>

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
enum OpcodeType
{
    OpcodeType_CONTINUATION = 0x0,
    OpcodeType_TEXT = 0x1,
    OpcodeType_BINARY = 0x2,
    OpcodeType_RSV3 = 0x3,
    OpcodeType_RSV4 = 0x4,
    OpcodeType_RSV5 = 0x5,
    OpcodeType_RSV6 = 0x6,
    OpcodeType_RSV7 = 0x7,
    OpcodeType_CLOSE = 0x8,
    OpcodeType_PING = 0x9,
    OpcodeType_PONG = 0xA,
    OpcodeType_CONTROL_RSVB = 0xB,
    OpcodeType_CONTROL_RSVC = 0xC,
    OpcodeType_CONTROL_RSVD = 0xD,
    OpcodeType_CONTROL_RSVE = 0xE,
    OpcodeType_CONTROL_RSVF = 0xF
};

class WebsocketFrame
{
public:
    // 1表示最后一帧
    uint8_t finish;
    // 三个扩展字段，应为0
    uint8_t rsv1;
    uint8_t rsv2;
    uint8_t rsv3;
    
    OpcodeType opcode;

    uint8_t mask;
    uint64_t payloadLen;
    uint64_t exPayloadLen;

    string maskKey;
    char* payload;
};

class WebsocketContext
{
public:
    void decode(char *data, size_t len);
    void encodeHeader(const WebsocketFrame& frame, StringBuffer::Ptr& header);
    void encodePayload(const WebsocketFrame& frame, Buffer::Ptr& payload);

    void setOnWebsocketFrame(const function<void(const char* data, int len)>& cb);
    void onWebsocketFrame(const char* data, int len);

public:
    WebsocketFrame _frame;

private:
    int _stage = 1; //1:header 2 byte, 2:header other, 3:payload
    int _otherHeaderSize = 0;
    uint32_t _maskIndex = 0;
    StringBuffer _remainData;
    function<void(const char* data, int len)> _onWebsocketFrame;
};



#endif //GB28181Parser_H
