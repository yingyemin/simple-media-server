#include "T9201.h"

// 构造函数，初始化成员变量为默认值
T9201::T9201()
    : ip(""),
      tcpPort(0),
      udpPort(0),
      channelNo(0),
      mediaType(0),
      streamType(0),
      storageType(0),
      playbackMode(0),
      playbackSpeed(0),
      startTime(""),
      endTime("") {}

// 获取服务器 IP 地址
std::string T9201::getIp() const {
    return ip;
}

// 获取实时视频服务器 TCP 端口号
uint16_t T9201::getTcpPort() const {
    return tcpPort;
}

// 获取实时视频服务器 UDP 端口号
uint16_t T9201::getUdpPort() const {
    return udpPort;
}

// 获取逻辑通道号
uint8_t T9201::getChannelNo() const {
    return channelNo;
}

// 获取音视频资源类型
uint8_t T9201::getMediaType() const {
    return mediaType;
}

// 获取码流类型
uint8_t T9201::getStreamType() const {
    return streamType;
}

// 获取存储器类型
uint8_t T9201::getStorageType() const {
    return storageType;
}

// 获取回放方式
uint8_t T9201::getPlaybackMode() const {
    return playbackMode;
}

// 获取快进或快退倍数
uint8_t T9201::getPlaybackSpeed() const {
    return playbackSpeed;
}

// 获取开始时间
std::string T9201::getStartTime() const {
    return startTime;
}

// 获取结束时间
std::string T9201::getEndTime() const {
    return endTime;
}

// 设置服务器 IP 地址
void T9201::setIp(const std::string& ip) {
    this->ip = ip;
}

// 设置实时视频服务器 TCP 端口号
void T9201::setTcpPort(uint16_t port) {
    tcpPort = port;
}

// 设置实时视频服务器 UDP 端口号
void T9201::setUdpPort(uint16_t port) {
    udpPort = port;
}

// 设置逻辑通道号
void T9201::setChannelNo(uint8_t no) {
    channelNo = no;
}

// 设置音视频资源类型
void T9201::setMediaType(uint8_t type) {
    mediaType = type;
}

// 设置码流类型
void T9201::setStreamType(uint8_t type) {
    streamType = type;
}

// 设置存储器类型
void T9201::setStorageType(uint8_t type) {
    storageType = type;
}

// 设置回放方式
void T9201::setPlaybackMode(uint8_t mode) {
    playbackMode = mode;
}

// 设置快进或快退倍数
void T9201::setPlaybackSpeed(uint8_t speed) {
    playbackSpeed = speed;
}

// 设置开始时间
void T9201::setStartTime(const std::string& time) {
    startTime = time;
}

// 设置结束时间
void T9201::setEndTime(const std::string& time) {
    endTime = time;
}
