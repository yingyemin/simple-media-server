#include "T8800.h"

// 默认构造函数
T8800::T8800() : mediaId(0) {}

// Getter 方法实现
int T8800::getMediaId() const {
    return mediaId;
}

const std::vector<short>& T8800::getId() const {
    return id;
}

// Setter 方法实现，支持链式调用
T8800& T8800::setMediaId(int newMediaId) {
    mediaId = newMediaId;
    return *this;
}

T8800& T8800::setId(const std::vector<short>& newId) {
    id = newId;
    return *this;
}

// 添加重传包 ID 方法实现
T8800& T8800::addId(short newId) {
    id.push_back(newId);
    return *this;
}
