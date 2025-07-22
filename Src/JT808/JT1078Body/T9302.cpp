#include "T9302.h"

// 构造函数，初始化成员变量为默认值
T9302::T9302() : channelNo(0), param(0) {}

// 获取逻辑通道号
uint8_t T9302::getChannelNo() const {
    return channelNo;
}

// 获取控制参数
uint8_t T9302::getParam() const {
    return param;
}

// 设置逻辑通道号
void T9302::setChannelNo(uint8_t no) {
    channelNo = no;
}

// 设置控制参数
void T9302::setParam(uint8_t param) {
    this->param = param;
}
