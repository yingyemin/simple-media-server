#ifndef T9101_H
#define T9101_H

#include <cstdint>
#include <string>

#include "Net/Buffer.h"

// 模拟 JTMessage 类，这里简单用空类表示
class JTMessage {};

// 实时音视频传输请求类
class T9101 : public JTMessage {
public:
    // 构造函数
    T9101();

    // Getter 和 Setter 方法
    std::string getIp() const;
    void setIp(const std::string& ip);

    uint16_t getTcpPort() const;
    void setTcpPort(uint16_t port);

    uint16_t getUdpPort() const;
    void setUdpPort(uint16_t port);

    uint8_t getChannelNo() const;
    void setChannelNo(uint8_t no);

    uint8_t getMediaType() const;
    void setMediaType(uint8_t type);

    uint8_t getStreamType() const;
    void setStreamType(uint8_t type);

    void parse(const char* data, int len);
    StringBuffer::Ptr encode();

private:
    // ip的长度，占一个字节
    int ipLength;
    // 服务器 IP 地址，实际传输长度按需确定
    std::string ip;
    // 实时视频服务器 TCP 端口号，占用 2 字节
    uint16_t tcpPort;
    // 实时视频服务器 UDP 端口号，占用 2 字节
    uint16_t udpPort;
    // 逻辑通道号，占用 1 字节
    uint8_t channelNo;
    // 数据类型：0.音视频 1.视频 2.双向对讲 3.监听 4.中心广播 5.透传，占用 1 字节
    uint8_t mediaType;
    // 码流类型：0.主码流 1.子码流，占用 1 字节
    uint8_t streamType;
};

#endif // T9101_H
