#include "T0705.h"

// T0705 默认构造函数
T0705::T0705() : total(0) {}

// T0705 析构函数
T0705::~T0705() = default;

// T0705 Getter 方法实现
int T0705::getTotal() const {
    return total;
}

const std::string& T0705::getDateTime() const {
    return dateTime;
}

const std::vector<T0705::Item>& T0705::getItems() const {
    return items;
}

// T0705 Setter 方法实现，支持链式调用
T0705& T0705::setTotal(int newTotal) {
    total = newTotal;
    return *this;
}

T0705& T0705::setDateTime(const std::string& newDateTime) {
    dateTime = newDateTime;
    return *this;
}

T0705& T0705::setItems(const std::vector<Item>& newItems) {
    items = newItems;
    total = static_cast<int>(items.size());
    return *this;
}

// Item 默认构造函数
T0705::Item::Item() : id(0) {}

// Item 析构函数
T0705::Item::~Item() = default;

// Item Getter 方法实现
int T0705::Item::getId() const {
    return id;
}

const std::vector<unsigned char>& T0705::Item::getData() const {
    return data;
}

// Item Setter 方法实现，支持链式调用
T0705::Item& T0705::Item::setId(int newId) {
    id = newId;
    return *this;
}

T0705::Item& T0705::Item::setData(const std::vector<unsigned char>& newData) {
    data = newData;
    return *this;
}
