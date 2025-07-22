#include "T0802.h"

// 默认构造函数
T0802::T0802() : responseSerialNo(0) {}

// 析构函数
T0802::~T0802() = default;

// Getter 方法实现
int T0802::getResponseSerialNo() const {
    return responseSerialNo;
}

const std::vector<std::unique_ptr<Item>>& T0802::getItems() const {
    return items;
}

// Setter 方法实现，支持链式调用
T0802& T0802::setResponseSerialNo(int newResponseSerialNo) {
    responseSerialNo = newResponseSerialNo;
    return *this;
}

T0802& T0802::addItem(std::unique_ptr<Item> newItem) {
    items.push_back(std::move(newItem));
    return *this;
}
