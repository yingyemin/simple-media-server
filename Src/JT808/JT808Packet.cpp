#include "JT808Packet.h"
#include "Log/Logger.h"

JT808Packet::JT808Packet() {
}

JT808Packet::~JT808Packet() {
}

StringBuffer::Ptr JT808Packet::pack(JT808Header& header, const StringBuffer::Ptr& body)
{
    StringBuffer::Ptr buffer = std::make_shared<StringBuffer>();
    buffer->push_back((header.msgId >> 8) & 0xFF);
    buffer->push_back(header.msgId & 0xFF);
    header.bodyLength = body->size();

    //消息体属性构造
    uint16_t bprops = (header.bodyLength & 0x03FF) |
                      (header.isEncrypted ? 0x0400 : 0) |
                      (header.isPacketSplit ? 0x0020 : 0) |
                      (header.versionFlag << 14);
                      
    buffer->push_back((bprops >> 8) & 0xFF);
    buffer->push_back(bprops & 0xFF);

    if (header.versionFlag)
    {
        buffer->push_back(header.version);
    }
    buffer->append(header.simCode);
    buffer->push_back((header.serialNo >> 8) & 0xFF);
    buffer->push_back(header.serialNo & 0xFF);

    if (header.isPacketSplit)
    {
        buffer->push_back((header.packSum >> 8) & 0xFF);
        buffer->push_back(header.packSum & 0xFF);
        buffer->push_back((header.packSeq >> 8) & 0xFF);
        buffer->push_back(header.packSeq & 0xFF);
    }

    if (body)
    {
        buffer->append(*body);
    }

    buffer->push_back(JT808Packet::calcCheckSum((const unsigned char*)buffer->data(), buffer->size()));
    auto escapedData = JT808Packet::escape((const unsigned char*)buffer->data(), buffer->size());

    return escapedData;
}


/**
 * 转义还原，消息头、消息体、校验码中均可能包含转义字符
 * @param data 收到的原始消息
 * @param length 待解码数据长度
 * @param unescapedData 解码后数据
 * @return 解码后数据长度，解码后消息包含：消息头\消息体\校验码
 */
size_t JT808Packet::unescape(const unsigned char* data, size_t length,unsigned char* unescapedData) {
    size_t unescapedLength = 0;
    for (size_t i = 0; i < length; i++) {
        if (data[i] == ESCAPE_CHAR && i + 1 < length) {
            if (data[i + 1] == ESCAPE_XOR_1) {
                unescapedData[unescapedLength++] = ESCAPE_CHAR;
                ++i;
            } else if (data[i + 1] == ESCAPE_XOR_2) {
                unescapedData[unescapedLength++] = MESSAGE_START_END_FLAG;
                ++i;
            } else {
                //错误处理：非法的转义序列
                logError << "Invalid escape sequence found!";
                return 0;   //或者可以选择抛出异常
            }
        } else {
            unescapedData[unescapedLength++] = data[i];
        }
    }
    return unescapedLength;
}

StringBuffer::Ptr JT808Packet::escape(const unsigned char* data, size_t length) {
    StringBuffer::Ptr unescapedData = std::make_shared<StringBuffer>();
    unescapedData->push_back(MESSAGE_START_END_FLAG);
    for (size_t i = 0; i < length; i++) {
        if (data[i] == ESCAPE_CHAR) {
            unescapedData->push_back(ESCAPE_CHAR);
            unescapedData->push_back(ESCAPE_XOR_1);
        } else if (data[i] == MESSAGE_START_END_FLAG) {
            unescapedData->push_back(ESCAPE_CHAR);
            unescapedData->push_back(ESCAPE_XOR_2);
        }else{
            unescapedData->push_back(data[i]);
        }
    }
    unescapedData->push_back(MESSAGE_START_END_FLAG);
    return unescapedData;
}

//计算校验和的函数
unsigned char JT808Packet::calcCheckSum(const unsigned char* buffer, size_t bufferSize) {
    unsigned char checksum = 0;
    for (size_t i = 0; i < bufferSize; i++) {
        checksum ^= buffer[i]; //对当前校验和与缓冲区中的下一个字节进行异或操作
    }
    return checksum;
}


//解析BCD编码的手机号为十进制字符串
std::string JT808Packet::decodeBCDPhoneNumber(const std::string& bcdPhone,bool removePreZero) {
    std::string phoneNumber;
    for (size_t i = 0; i < bcdPhone.size(); ++i) {
        phoneNumber += ((bcdPhone[i] & 0xf0) >> 4) + '0'; //取高位并转换为字符
        phoneNumber += (bcdPhone[i] & 0x0f) + '0';       //取低位并转换为字符
    }
    if(removePreZero){
        //去除前导零
        size_t nonZeroPos = phoneNumber.find_first_not_of('0');
        if (nonZeroPos != std::string::npos) {
            phoneNumber = phoneNumber.substr(nonZeroPos);
        }
    }
    return phoneNumber;
}