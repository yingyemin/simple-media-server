#include "T9205.h"

// 构造函数实现
T9205::T9205()
    : channelNo(0),
      startTime(""),
      endTime(""),
      warnBit1(0),
      warnBit2(0),
      mediaType(0),
      streamType(0),
      storageType(0) {}

// Getter 方法实现
uint8_t T9205::getChannelNo() const {
    return channelNo;
}

std::string T9205::getStartTime() const {
    return startTime;
}

std::string T9205::getEndTime() const {
    return endTime;
}

uint32_t T9205::getWarnBit1() const {
    return warnBit1;
}

uint32_t T9205::getWarnBit2() const {
    return warnBit2;
}

uint8_t T9205::getMediaType() const {
    return mediaType;
}

uint8_t T9205::getStreamType() const {
    return streamType;
}

uint8_t T9205::getStorageType() const {
    return storageType;
}

// Setter 方法实现
void T9205::setChannelNo(uint8_t no) {
    channelNo = no;
}

void T9205::setStartTime(const std::string& time) {
    startTime = time;
}

void T9205::setEndTime(const std::string& time) {
    endTime = time;
}

void T9205::setWarnBit1(uint32_t bit) {
    warnBit1 = bit;
}

void T9205::setWarnBit2(uint32_t bit) {
    warnBit2 = bit;
}

void T9205::setMediaType(uint8_t type) {
    mediaType = type;
}

void T9205::setStreamType(uint8_t type) {
    streamType = type;
}

void T9205::setStorageType(uint8_t type) {
    storageType = type;
}
