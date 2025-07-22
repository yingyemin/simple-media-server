#include "T1205.h"
#include <ctime>

// Item 类构造函数
Item::Item() :
    channelNo(0),
    warnBit(0),
    mediaType(0),
    streamType(1),
    storageType(0),
    size(0) {}

// 获取逻辑通道号
uint8_t Item::getChannelNo() const {
    return channelNo;
}

// 设置逻辑通道号
void Item::setChannelNo(uint8_t no) {
    channelNo = no;
}

// 获取开始时间
std::chrono::system_clock::time_point Item::getStartTime() const {
    return startTime;
}

// 设置开始时间
void Item::setStartTime(const std::chrono::system_clock::time_point& time) {
    startTime = time;
}

// 获取结束时间
std::chrono::system_clock::time_point Item::getEndTime() const {
    return endTime;
}

// 设置结束时间
void Item::setEndTime(const std::chrono::system_clock::time_point& time) {
    endTime = time;
}

// 获取报警标志
uint64_t Item::getWarnBit() const {
    return warnBit;
}

// 设置报警标志
void Item::setWarnBit(uint64_t bit) {
    warnBit = bit;
}

// 获取音视频资源类型
uint8_t Item::getMediaType() const {
    return mediaType;
}

// 设置音视频资源类型
void Item::setMediaType(uint8_t type) {
    mediaType = type;
}

// 获取码流类型
uint8_t Item::getStreamType() const {
    return streamType;
}

// 设置码流类型
void Item::setStreamType(uint8_t type) {
    streamType = type;
}

// 获取存储器类型
uint8_t Item::getStorageType() const {
    return storageType;
}

// 设置存储器类型
void Item::setStorageType(uint8_t type) {
    storageType = type;
}

// 获取文件大小
uint32_t Item::getSize() const {
    return size;
}

// 设置文件大小
void Item::setSize(uint32_t size) {
    this->size = size;
}

// T1205 类构造函数
T1205::T1205() :
    responseSerialNo(0) {}

// 获取应答流水号
uint16_t T1205::getResponseSerialNo() const {
    return responseSerialNo;
}

// 设置应答流水号
void T1205::setResponseSerialNo(uint16_t no) {
    responseSerialNo = no;
}

// 获取音视频资源列表
std::vector<Item> T1205::getItems() const {
    return items;
}

// 设置音视频资源列表
void T1205::setItems(const std::vector<Item>& items) {
    this->items = items;
}
