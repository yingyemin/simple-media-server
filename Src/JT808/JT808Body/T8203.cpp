#include "T8203.h"

// 默认构造函数
T8203::T8203() : responseSerialNo(0), type(0) {}

// 析构函数
T8203::~T8203() = default;

// Getter 方法实现
int T8203::getResponseSerialNo() const {
    return responseSerialNo;
}

int T8203::getType() const {
    return type;
}

// Setter 方法实现，支持链式调用
T8203& T8203::setResponseSerialNo(int newResponseSerialNo) {
    responseSerialNo = newResponseSerialNo;
    return *this;
}

T8203& T8203::setType(int newType) {
    type = newType;
    return *this;
}
