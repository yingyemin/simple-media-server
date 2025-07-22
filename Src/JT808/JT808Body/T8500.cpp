#include "T8500.h"

// 默认构造函数
T8500::T8500() : type(0), param(0) {}

// 析构函数
T8500::~T8500() = default;

// Getter 方法实现
int T8500::getType() const {
    return type;
}

int T8500::getParam() const {
    return param;
}

// Setter 方法实现，支持链式调用
T8500& T8500::setType(int newType) {
    type = newType;
    return *this;
}

T8500& T8500::setParam(int newParam) {
    param = newParam;
    return *this;
}
