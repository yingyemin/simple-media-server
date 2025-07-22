#include "T8202.h"

// 默认构造函数
T8202::T8202() : interval(0), validityPeriod(0) {}

// 析构函数
T8202::~T8202() = default;

// Getter 方法实现
int T8202::getInterval() const {
    return interval;
}

int T8202::getValidityPeriod() const {
    return validityPeriod;
}

// Setter 方法实现，支持链式调用
T8202& T8202::setInterval(int newInterval) {
    interval = newInterval;
    return *this;
}

T8202& T8202::setValidityPeriod(int newValidityPeriod) {
    validityPeriod = newValidityPeriod;
    return *this;
}
