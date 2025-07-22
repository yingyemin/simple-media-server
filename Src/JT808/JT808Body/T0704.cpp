#include "T0704.h"
#include <algorithm>

// 默认构造函数
T0704::T0704() : total(0), type(0) {}

// 析构函数，负责释放 items 里的 T0200 对象
T0704::~T0704() {
}

// Getter 方法实现
int T0704::getTotal() const {
    return total;
}

int T0704::getType() const {
    return type;
}

const std::vector<std::shared_ptr<T0200_VersionMinus1And0>>& T0704::getItems() const {
    return items;
}

const std::vector<std::shared_ptr<T0200_Version1>>& T0704::getItems2019() const {
    return items2019;
}

// Setter 方法实现，支持链式调用
T0704& T0704::setTotal(int newTotal) {
    total = newTotal;
    return *this;
}

T0704& T0704::setType(int newType) {
    type = newType;
    return *this;
}

T0704& T0704::setItems(const std::vector<std::shared_ptr<T0200_VersionMinus1And0>>& newItems) {
    items.clear();
    // 复制新的指针
    items.reserve(newItems.size());
    for (std::shared_ptr<T0200_VersionMinus1And0> item : newItems) {
        items.push_back(item);
    }
    total = static_cast<int>(items.size());
    return *this;
}

T0704& T0704::setItems2019(const std::vector<std::shared_ptr<T0200_Version1>>& newItems) {
    items2019.clear();
    // 复制新的指针
    items2019.reserve(newItems.size());
    for (std::shared_ptr<T0200_Version1> item : newItems) {
        items2019.push_back(item);
    }
    total = static_cast<int>(items2019.size());
    return *this;
}

// 添加单个 T0200 元素的方法
T0704& T0704::addItem(std::shared_ptr<T0200_Version1> item) {
    items2019.push_back(item);
    total = static_cast<int>(items2019.size());
    return *this;
}

// 添加单个 T0200 元素的方法
T0704& T0704::addItem(std::shared_ptr<T0200_VersionMinus1And0> item) {
    items.push_back(item);
    total = static_cast<int>(items.size());
    return *this;
}
