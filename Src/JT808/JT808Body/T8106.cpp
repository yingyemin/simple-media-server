#include "T8106.h"

// 默认构造函数
T8106::T8106() = default;

// 析构函数
T8106::~T8106() = default;

// Getter 方法实现
const std::vector<int>& T8106::getId() const {
    return id;
}

// Setter 方法实现，支持链式调用
T8106& T8106::setId(const std::vector<int>& newId) {
    id = newId;
    return *this;
}

T8106& T8106::addId(int newId) {
    id.push_back(newId);
    return *this;
}
