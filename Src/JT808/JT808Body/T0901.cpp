#include "T0901.h"

// 默认构造函数
T0901::T0901() = default;

// 析构函数
T0901::~T0901() = default;

// Getter 方法实现
const std::vector<unsigned char>& T0901::getBody() const {
    return body;
}

// Setter 方法实现，支持链式调用
T0901& T0901::setBody(const std::vector<unsigned char>& newBody) {
    body = newBody;
    return *this;
}
