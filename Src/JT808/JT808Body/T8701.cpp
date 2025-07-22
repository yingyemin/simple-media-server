#include "T8701.h"

// 默认构造函数
T8701::T8701() : type(0) {}

// Getter 方法实现
int T8701::getType() const {
    return type;
}

const std::vector<unsigned char>& T8701::getContent() const {
    return content;
}

// Setter 方法实现，支持链式调用
T8701& T8701::setType(int newType) {
    type = newType;
    return *this;
}

T8701& T8701::setContent(const std::vector<unsigned char>& newContent) {
    content = newContent;
    return *this;
}

// 添加数据字节方法实现
T8701& T8701::addContentByte(unsigned char byte) {
    content.push_back(byte);
    return *this;
}
