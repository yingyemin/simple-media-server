#include "T8304.h"

// 默认构造函数
T8304::T8304() : type(0), content("") {}

// 析构函数
T8304::~T8304() = default;

// Getter 方法实现
int T8304::getType() const {
    return type;
}

const std::string& T8304::getContent() const {
    return content;
}

// Setter 方法实现，支持链式调用
T8304& T8304::setType(int newType) {
    type = newType;
    return *this;
}

T8304& T8304::setContent(const std::string& newContent) {
    content = newContent;
    return *this;
}
