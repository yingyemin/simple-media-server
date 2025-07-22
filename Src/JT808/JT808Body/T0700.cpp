#include "T0700.h"

// 默认构造函数
T0700::T0700() : responseSerialNo(0), command(0) {}

// 析构函数
T0700::~T0700() = default;

// 获取应答流水号
int T0700::getResponseSerialNo() const {
    return responseSerialNo;
}

// 设置应答流水号，支持链式调用
T0700& T0700::setResponseSerialNo(int serialNo) {
    responseSerialNo = serialNo;
    return *this;
}

// 获取命令字
int T0700::getCommand() const {
    return command;
}

// 设置命令字，支持链式调用
T0700& T0700::setCommand(int cmd) {
    command = cmd;
    return *this;
}

// 获取数据块
const std::vector<unsigned char>& T0700::getData() const {
    return data;
}

// 设置数据块，支持链式调用
T0700& T0700::setData(const std::vector<unsigned char>& newData) {
    data = newData;
    return *this;
}
