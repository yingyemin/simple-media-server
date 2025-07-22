#include "T9202.h"

// 构造函数，初始化成员变量
T9202::T9202() :
    channelNo(0),
    playbackMode(0),
    playbackSpeed(0),
    playbackTime("") {}

// 获取逻辑通道号
uint8_t T9202::getChannelNo() const {
    return channelNo;
}

// 设置逻辑通道号
void T9202::setChannelNo(uint8_t no) {
    channelNo = no;
}

// 获取回放控制
uint8_t T9202::getPlaybackMode() const {
    return playbackMode;
}

// 设置回放控制
void T9202::setPlaybackMode(uint8_t mode) {
    playbackMode = mode;
}

// 获取快进或快退倍数
uint8_t T9202::getPlaybackSpeed() const {
    return playbackSpeed;
}

// 设置快进或快退倍数
void T9202::setPlaybackSpeed(uint8_t speed) {
    playbackSpeed = speed;
}

// 获取拖动回放位置
std::string T9202::getPlaybackTime() const {
    return playbackTime;
}

// 设置拖动回放位置
void T9202::setPlaybackTime(const std::string& time) {
    playbackTime = time;
}
