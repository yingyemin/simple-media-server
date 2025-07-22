#include "T0900.h"

// 默认构造函数
T0900::T0900() = default;

// 析构函数
T0900::~T0900() = default;

// Getter 方法
uint8_t T0900::getType() const {
    return type;
}

uint32_t T0900::getTotalLength() const {
    return totalLength;
}

uint16_t T0900::getMsgLength() const {
    return msgLength;
}

const std::vector<uint8_t>& T0900::getMsgData() const {
    return msgData;
}

// Setter 方法
T0900& T0900::setType(uint8_t newType) {
    type = newType;
    return *this;
}

T0900& T0900::setTotalLength(uint32_t newTotalLength) {
    totalLength = newTotalLength;
    return *this;
}

T0900& T0900::setMsgLength(uint16_t newMsgLength) {
    msgLength = newMsgLength;
    return *this;
}

T0900& T0900::setMsgData(const std::vector<uint8_t>& newMsgData) {
    msgData = newMsgData;
    return *this;
}