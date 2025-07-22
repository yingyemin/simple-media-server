#include "T8004.h"

// 默认构造函数，初始化为当前时间
T8004::T8004() : dateTime(std::chrono::system_clock::now()) {}

// 析构函数
T8004::~T8004() = default;

// Getter 方法实现
std::chrono::system_clock::time_point T8004::getDateTime() const {
    return dateTime;
}

// Setter 方法实现，支持链式调用
T8004& T8004::setDateTime(const std::chrono::system_clock::time_point& newDateTime) {
    dateTime = newDateTime;
    return *this;
}
