#include "T9301.h"

// 构造函数，初始化成员变量为默认值
T9301::T9301() : channelNo(0), direction(0), speed(0) {}

// 获取逻辑通道号
uint8_t T9301::getChannelNo() const {
    return channelNo;
}

// 获取云台旋转方向
uint8_t T9301::getDirection() const {
    return direction;
}

// 获取云台旋转速度
uint8_t T9301::getSpeed() const {
    return speed;
}

// 设置逻辑通道号
void T9301::setChannelNo(uint8_t no) {
    channelNo = no;
}

// 设置云台旋转方向
void T9301::setDirection(uint8_t direction) {
    this->direction = direction;
}

// 设置云台旋转速度
void T9301::setSpeed(uint8_t speed) {
    this->speed = speed;
}
