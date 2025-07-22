#include "T9101.h"

// 构造函数，初始化成员变量
T9101::T9101() :
    ip(""),
    tcpPort(0),
    udpPort(0),
    channelNo(0),
    mediaType(0),
    streamType(0) {}

// 获取服务器 IP 地址
std::string T9101::getIp() const {
    return ip;
}

// 设置服务器 IP 地址
void T9101::setIp(const std::string& ip) {
    this->ip = ip;
}

// 获取实时视频服务器 TCP 端口号
uint16_t T9101::getTcpPort() const {
    return tcpPort;
}

// 设置实时视频服务器 TCP 端口号
void T9101::setTcpPort(uint16_t port) {
    tcpPort = port;
}

// 获取实时视频服务器 UDP 端口号
uint16_t T9101::getUdpPort() const {
    return udpPort;
}

// 设置实时视频服务器 UDP 端口号
void T9101::setUdpPort(uint16_t port) {
    udpPort = port;
}

// 获取逻辑通道号
uint8_t T9101::getChannelNo() const {
    return channelNo;
}

// 设置逻辑通道号
void T9101::setChannelNo(uint8_t no) {
    channelNo = no;
}

// 获取数据类型
uint8_t T9101::getMediaType() const {
    return mediaType;
}

// 设置数据类型
void T9101::setMediaType(uint8_t type) {
    mediaType = type;
}

// 获取码流类型
uint8_t T9101::getStreamType() const {
    return streamType;
}

// 设置码流类型
void T9101::setStreamType(uint8_t type) {
    streamType = type;
}

// 解析传入数据到类成员
void T9101::parse(const char* data, int len) {
    if (data == nullptr || len < 7) {
        return;
    }
    
    // 解析IP长度
    ipLength = static_cast<unsigned char>(data[0]);
    
    // 解析IP地址
    if (len >= 1 + ipLength) {
        ip = std::string(data + 1, ipLength);
    }
    
    // 解析TCP端口号
    if (len >= 3 + ipLength) {
        tcpPort = static_cast<uint16_t>((static_cast<unsigned char>(data[1 + ipLength]) << 8) | static_cast<unsigned char>(data[2 + ipLength]));
    }
    
    // 解析UDP端口号
    if (len >= 5 + ipLength) {
        udpPort = static_cast<uint16_t>((static_cast<unsigned char>(data[3 + ipLength]) << 8) | static_cast<unsigned char>(data[4 + ipLength]));
    }
    
    // 解析逻辑通道号
    if (len >= 6 + ipLength) {
        channelNo = static_cast<unsigned char>(data[5 + ipLength]);
    }
    
    // 解析数据类型
    if (len >= 7 + ipLength) {
        mediaType = static_cast<unsigned char>(data[6 + ipLength]);
    }
    
    // 解析码流类型
    if (len >= 8 + ipLength) {
        streamType = static_cast<unsigned char>(data[7 + ipLength]);
    }
}

// 将类成员编码成字节流
StringBuffer::Ptr T9101::encode() {
    auto buffer = make_shared<StringBuffer>();
    
    // 写入IP长度
    buffer->push_back(static_cast<char>(ip.size()));
    
    // 写入IP地址
    buffer->append(ip.c_str(), ip.size());
    
    // 写入TCP端口号
    buffer->push_back(static_cast<char>(tcpPort >> 8));
    buffer->push_back(static_cast<char>(tcpPort & 0xFF));
    
    // 写入UDP端口号
    buffer->push_back(static_cast<char>(udpPort >> 8));
    buffer->push_back(static_cast<char>(udpPort & 0xFF));
    
    // 写入逻辑通道号
    buffer->push_back(static_cast<char>(channelNo));
    
    // 写入数据类型
    buffer->push_back(static_cast<char>(mediaType));
    
    // 写入码流类型
    buffer->push_back(static_cast<char>(streamType));
    
    return buffer;
}