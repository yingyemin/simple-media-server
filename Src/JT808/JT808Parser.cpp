#include "JT808Parser.h"
#include "Log/Logger.h"

using namespace std;

JT808Parser::JT808Parser() {
    _pkt = make_shared<JT808Packet>();
}

JT808Parser::~JT808Parser() {
}

//解析JT808消息头
bool JT808Parser::parseHeader(const unsigned char* buffer, size_t bufferSize, JT808Header& header) {
    // if (bufferSize < HEADER_MIN_LENGTH) {
    //     logError << "Buffer too small to contain a minimal JT808 header.";
    //     return false;
    // }

    int index = 0;
    //大端字节序读取
    header.msgId = (buffer[index++] << 8) | buffer[index++];
    header.properties = (buffer[index++] << 8) | buffer[index++];
    //解析消息体属性
    header.bodyLength = header.properties & LENGTH_MASK;     //消息体长度
    header.isEncrypted = (header.properties   >> 10) & 0x01; //加密标识
    header.isPacketSplit = (header.properties >> 13) & 0x01; //分包标识，1表示是分包消息，0表示整包消息
    header.versionFlag = (header.properties   >> 14) & 0x01;    

    if (header.versionFlag) {
        header.version = buffer[index++];
    }
    
    int simCodeLen = 6;
    if (header.versionFlag) {
        simCodeLen = 10;
    }
    //BCD解码手机号
    for(int i = 0; i < simCodeLen; ++i) {
        header.simCode.push_back(buffer[index++]);
    }
    
    header.serialNo = (buffer[index++] << 24) | (buffer[index++] << 16) | (buffer[index++] << 8) | buffer[index++];

     

    //判断是否有分包信息
    // if(header.isPacketSplit && bufferSize<HEADER_MAX_LENGTH) {
    //     logError << "This message is packeted, but has no pack info.";
    //     return false;
    // }
    if(header.isPacketSplit) {
        header.packSum = (buffer[index++] << 8) | buffer[index++];
        header.packSeq = (buffer[index++] << 8) | buffer[index++];
    }
    return true;
}

void JT808Parser::parse(const char *buffer, size_t len)
{
    // 从配置中获取最大缓存大小
    static int maxRemainSize = 4 * 1024 * 1024;

    int remainSize = _remainData.size();
    if (remainSize > maxRemainSize) {
        logError << "remain cache is too large";
        _remainData.clear();
        return ;
    }

    auto unescapedData = make_shared<StreamBuffer>(len + 1);
    len = JT808Packet::unescape((unsigned char *)buffer, len,(unsigned char*)unescapedData->data());

    char* data = unescapedData->data();

    if (remainSize > 0) {
        _remainData.append(data, len);
        data = _remainData.data();
        len += remainSize;
    }

    auto start = data;
    auto end = data + len;
    
    while (data < end) {
        if (data[0] != MESSAGE_START_END_FLAG) {
            start = end;
            break;
        }

        if (end - data < 4 + 1) {
            break;
        }

        uint16_t properties = (data[3] << 8) | data[4];
        uint8_t versionFlag = (properties >> 14) & 0x01;
        bool isPacketSplit = (properties >> 13) & 0x01;
        uint16_t bodyLength = properties & LENGTH_MASK;

        uint16_t headerLength = isPacketSplit ? 4 : 0;
        headerLength += versionFlag ? 17 : 12;

        uint16_t totalLength = headerLength + bodyLength + 2 + 1;

        if (end - data < totalLength) {
            break;
        }

        if (data[totalLength-1] != MESSAGE_START_END_FLAG) {
            start = end;
            break;
        }

        //buffer的最后一位是校验码
        unsigned char checksum = JT808Packet::calcCheckSum((unsigned char*)data + 1,totalLength-3);
        if(checksum!=(unsigned char)data[totalLength-2]){
            logError << "Message check sum error!" << (int)checksum << ":" << (int)data[totalLength-1] << std::endl;
            break ;
        }

        parseHeader((unsigned char*)data + 1, totalLength - 3, _pkt->_header);
        _pkt->_header.headerLength = headerLength;
        _pkt->_content.append(data + headerLength + 1, totalLength - headerLength - 3);

        if (_pkt->_header.packSeq == _pkt->_header.packSum) {
            onJT808Packet(_pkt);
            _pkt = make_shared<JT808Packet>();
        }

        data += totalLength;
    }

    if (data < end) {
        // logInfo << "have remain data: " << (end - data);
        _remainData.assign(data, end - data);
    } else {
        // logInfo << "don't have remain data";
        _remainData.clear();
    }
}

void JT808Parser::setOnJT808Packet(const function<void(const JT808Packet::Ptr& pkt)>& cb) {
    _onJT808Packet = cb;
}

void JT808Parser::onJT808Packet(const JT808Packet::Ptr& pkt) {
    if (_onJT808Packet) {
        _onJT808Packet(pkt);
    }
}