#ifndef T0102_H
#define T0102_H

#include <string>
#include <vector>

/**
 * 对应版本 -1 和 0 的 T0102 消息类
 */
class T0102VNeg1And0 {
private:
    std::string token; // 终端重连后上报鉴权码

public:
    // Getter 方法
    const std::string& getToken() const;

    // Setter 方法
    void setToken(const std::string& token);

    int getProtocolVersion() const;
    std::string toString() const;

    /**
     * 从字节流解析数据填充对象
     * @param buffer 字节流
     * @param offset 起始偏移量
     */
    void parse(const std::vector<uint8_t>& buffer, size_t& offset);

    /**
     * 将对象数据编码为字节流
     * @param buffer 用于存储编码后数据的字节流
     */
    void encode(std::vector<uint8_t>& buffer) const;
};

/**
 * 对应版本 1 的 T0102 消息类
 */
class T0102V1 {
private:
    uint8_t tokenLength; // token 长度
    std::string token; // 终端重连后上报鉴权码
    std::string imei; // 终端 IMEI，15 字节
    std::string softwareVersion; // 软件版本号，20 字节

public:
    // Getter 方法
    uint8_t getTokenLength() const;
    const std::string& getToken() const;
    const std::string& getImei() const;
    const std::string& getSoftwareVersion() const;

    // Setter 方法
    void setTokenLength(uint8_t length);
    void setToken(const std::string& token);
    void setImei(const std::string& imei);
    void setSoftwareVersion(const std::string& version);

    int getProtocolVersion() const;
    std::string toString() const;

    /**
     * 从字节流解析数据填充对象
     * @param buffer 字节流
     * @param offset 起始偏移量
     */
    void parse(const std::vector<uint8_t>& buffer, size_t& offset);

    /**
     * 将对象数据编码为字节流
     * @param buffer 用于存储编码后数据的字节流
     */
    void encode(std::vector<uint8_t>& buffer) const;
};

#endif // T0102_H
