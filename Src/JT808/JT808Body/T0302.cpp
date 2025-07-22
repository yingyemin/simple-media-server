#include "T0302.h"

// 默认构造函数
T0302::T0302() = default;
// 析构函数
T0302::~T0302() = default;

// 获取应答流水号
int T0302::getResponseSerialNo() const {
    return responseSerialNo;
}

// 设置应答流水号，支持链式调用
T0302& T0302::setResponseSerialNo(int serialNo) {
    responseSerialNo = serialNo;
    return *this;
}

// 获取答案 ID
int T0302::getAnswerId() const {
    return answerId;
}

// 设置答案 ID，支持链式调用
T0302& T0302::setAnswerId(int id) {
    answerId = id;
    return *this;
}
