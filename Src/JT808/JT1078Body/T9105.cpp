#include "T9105.h"

// 构造函数，初始化成员变量
T9105::T9105() : channelNo(0), packetLossRate(0) {}

// 获取逻辑通道号
uint8_t T9105::getChannelNo() const {
    return channelNo;
}

// 设置逻辑通道号
void T9105::setChannelNo(uint8_t no) {
    channelNo = no;
}

// 获取丢包率
uint8_t T9105::getPacketLossRate() const {
    return packetLossRate;
}

// 设置丢包率
void T9105::setPacketLossRate(uint8_t rate) {
    packetLossRate = rate;
}
