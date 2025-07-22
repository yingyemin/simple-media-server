#include "T9207.h"

// 构造函数，初始化成员变量为默认值
T9207::T9207() : responseSerialNo(0), command(0) {}

// 获取应答流水号
uint16_t T9207::getResponseSerialNo() const {
    return responseSerialNo;
}

// 获取上传控制指令
uint8_t T9207::getCommand() const {
    return command;
}

// 设置应答流水号
void T9207::setResponseSerialNo(uint16_t serialNo) {
    responseSerialNo = serialNo;
}

// 设置上传控制指令
void T9207::setCommand(uint8_t cmd) {
    command = cmd;
}
