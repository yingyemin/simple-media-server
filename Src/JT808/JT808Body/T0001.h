#ifndef T0001_H
#define T0001_H

#include <string>
#include <vector>
#include <cstdint>

#include "Net/Buffer.h"

// 终端通用应答
class T0001 {
private:
    static const uint8_t Success = 0; // 成功、确认
    static const uint8_t Failure = 1; // 失败
    static const uint8_t MessageError = 2; // 消息有误
    static const uint8_t NotSupport = 3; // 不支持
    static const uint8_t AlarmAck = 4; // 报警处理确认

    uint16_t responseSerialNo; // 应答流水号，2 字节
    uint16_t responseMessageId; // 应答 ID，2 字节
    uint8_t resultCode; // 结果：0.成功 1.失败 2.消息有误 3.不支持 4.报警处理确认，1 字节

public:
    // Getter 方法
    uint16_t getResponseSerialNo() const;
    uint16_t getResponseMessageId() const;
    uint8_t getResultCode() const;

    // Setter 方法
    void setResponseSerialNo(uint16_t id);
    void setResponseMessageId(uint16_t id);
    void setResultCode(uint8_t code);

    bool isSuccess() const;

    /**
     * 从字节流解析数据填充对象
     * @param buffer 字节流
     * @param offset 起始偏移量
     */
    void parse(const char* buffer, size_t offset);

    /**
     * 将对象数据编码为字节流
     * @param buffer 用于存储编码后数据的字节流
     */
    StringBuffer::Ptr encode();
};

#endif // T0001_H
