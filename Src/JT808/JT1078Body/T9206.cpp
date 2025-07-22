#include "T9206.h"

// 构造函数，初始化成员变量
T9206::T9206()
    : ip(""),
      port(0),
      username(""),
      password(""),
      path(""),
      channelNo(0),
      startTime(""),
      endTime(""),
      warnBit1(0),
      warnBit2(0),
      mediaType(0),
      streamType(0),
      storageType(0),
      condition(255) {}

// Getter 方法实现
std::string T9206::getIp() const {
    return ip;
}

uint16_t T9206::getPort() const {
    return port;
}

std::string T9206::getUsername() const {
    return username;
}

std::string T9206::getPassword() const {
    return password;
}

std::string T9206::getPath() const {
    return path;
}

uint8_t T9206::getChannelNo() const {
    return channelNo;
}

std::string T9206::getStartTime() const {
    return startTime;
}

std::string T9206::getEndTime() const {
    return endTime;
}

uint32_t T9206::getWarnBit1() const {
    return warnBit1;
}

uint32_t T9206::getWarnBit2() const {
    return warnBit2;
}

uint8_t T9206::getMediaType() const {
    return mediaType;
}

uint8_t T9206::getStreamType() const {
    return streamType;
}

uint8_t T9206::getStorageType() const {
    return storageType;
}

uint8_t T9206::getCondition() const {
    return condition;
}

// Setter 方法实现
void T9206::setIp(const std::string& ip) {
    this->ip = ip;
}

void T9206::setPort(uint16_t port) {
    port = port;
}

void T9206::setUsername(const std::string& username) {
    this->username = username;
}

void T9206::setPassword(const std::string& password) {
    this->password = password;
}

void T9206::setPath(const std::string& path) {
    this->path = path;
}

void T9206::setChannelNo(uint8_t no) {
    channelNo = no;
}

void T9206::setStartTime(const std::string& time) {
    startTime = time;
}

void T9206::setEndTime(const std::string& time) {
    endTime = time;
}

void T9206::setWarnBit1(uint32_t bit) {
    warnBit1 = bit;
}

void T9206::setWarnBit2(uint32_t bit) {
    warnBit2 = bit;
}

void T9206::setMediaType(uint8_t type) {
    mediaType = type;
}

void T9206::setStreamType(uint8_t type) {
    streamType = type;
}

void T9206::setStorageType(uint8_t type) {
    storageType = type;
}

void T9206::setCondition(uint8_t condition) {
    this->condition = condition;
}
