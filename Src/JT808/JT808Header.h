#ifndef JT808Header_H
#define JT808Header_H

#include <memory>
#include <cstdint>
#include <string>

class JT808Header {
public:
    // 消息ID
    uint16_t msgId;
    // 消息体属性
    uint16_t properties;
    // 协议版本号
    uint8_t version = 0;
    // 终端手机号,2011和2013版本占6字节，2019版本占10字节
    std::string simCode;
    // 消息流水号
    uint16_t serialNo;
    uint16_t packSum = 0;     // 消息包总数（可选）
    uint16_t packSeq = 0;     // 消息包序号（可选）
    //bprops消息体属性
    uint16_t bodyLength;
    bool isEncrypted ;    // 加密标识
    bool isPacketSplit;   // 分包标识，1表示是分包消息，0表示整包消息
    bool versionFlag;     //版本标识

    uint16_t headerLength; // 自定义字段，用于找到body的起始位置
public:
    // Getter and Setter for msgId
    virtual uint16_t getMsgId() const;
    virtual void setMsgId(uint16_t id);
    
    // Getter and Setter for properties
    virtual uint16_t getProperties() const;
    virtual void setProperties(uint16_t prop);
    
    // Getter and Setter for simCode
    virtual std::string getSimCode();
    virtual void setSimCode(const std::string& code);
    
    // Getter and Setter for serialNo
    virtual uint16_t getSerialNo() const;
    virtual void setSerialNo(uint16_t no);

    static bool isVersion(uint16_t properties) {
        return (properties & 0b0100000000000000) == 0b0100000000000000;
    }

    virtual ~JT808Header() = default;
};

#endif //JT808Header_H