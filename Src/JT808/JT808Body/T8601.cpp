#include "T8601.h"

// 默认构造函数
T8601::T8601() = default;

// 析构函数
T8601::~T8601() = default;

// Getter 方法实现
const std::vector<int>& T8601::getId() const {
    return id;
}

// Setter 方法实现，支持链式调用
T8601& T8601::setId(const std::vector<int>& newId) {
    id = newId;
    return *this;
}

// 添加区域 ID 方法实现
T8601& T8601::addId(int newId) {
    id.push_back(newId);
    return *this;
}
