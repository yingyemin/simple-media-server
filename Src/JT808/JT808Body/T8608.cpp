#include "T8608.h"

// 默认构造函数
T8608::T8608() : type(0) {}

// Getter 方法实现
int T8608::getType() const {
    return type;
}

const std::vector<int>& T8608::getId() const {
    return id;
}

// Setter 方法实现，支持链式调用
T8608& T8608::setType(int newType) {
    type = newType;
    return *this;
}

T8608& T8608::setId(const std::vector<int>& newId) {
    id = newId;
    return *this;
}

// 添加区域 ID 方法实现
T8608& T8608::addId(int newId) {
    id.push_back(newId);
    return *this;
}
