#include "T8300.h"

// 默认构造函数
T8300::T8300() : sign(0), type(0), content("") {}

// 析构函数
T8300::~T8300() = default;

// Getter 方法实现
int T8300::getSign() const {
    return sign;
}

int T8300::getType() const {
    return type;
}

const std::string& T8300::getContent() const {
    return content;
}

// Setter 方法实现，支持链式调用
T8300& T8300::setSign(int newSign) {
    sign = newSign;
    return *this;
}

T8300& T8300::setType(int newType) {
    type = newType;
    return *this;
}

T8300& T8300::setContent(const std::string& newContent) {
    content = newContent;
    return *this;
}
