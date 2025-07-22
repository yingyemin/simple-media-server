#include "T9102.h"

// 构造函数，初始化成员变量
T9102::T9102() :
    channelNo(0),
    command(0),
    closeType(0),
    streamType(0) {}

// 获取逻辑通道号
uint8_t T9102::getChannelNo() const {
    return channelNo;
}

// 设置逻辑通道号
void T9102::setChannelNo(uint8_t no) {
    channelNo = no;
}

// 获取控制指令
uint8_t T9102::getCommand() const {
    return command;
}

// 设置控制指令
void T9102::setCommand(uint8_t cmd) {
    command = cmd;
}

// 获取关闭音视频类型
uint8_t T9102::getCloseType() const {
    return closeType;
}

// 设置关闭音视频类型
void T9102::setCloseType(uint8_t type) {
    closeType = type;
}

// 获取切换码流类型
uint8_t T9102::getStreamType() const {
    return streamType;
}

// 设置切换码流类型
void T9102::setStreamType(uint8_t type) {
    streamType = type;
}

void T9102::parse(const char* buf, size_t size) {
    if (size < 4) {
        return;
    }
    channelNo = buf[0];
    command = buf[1];
    closeType = buf[2];
    streamType = buf[3];
}

StringBuffer::Ptr T9102::encode() {
    StringBuffer::Ptr buffer = std::make_shared<StringBuffer>();
    buffer->push_back(channelNo);
    buffer->push_back(command);
    buffer->push_back(closeType);
    buffer->push_back(streamType);
    return buffer;
}