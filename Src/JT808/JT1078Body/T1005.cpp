#include "T1005.h"

// 构造函数，初始化成员变量
T1005::T1005() :
    startTime(""),
    endTime(""),
    getOnCount(0),
    getOffCount(0) {}

// 获取起始时间
std::string T1005::getStartTime() const {
    return startTime;
}

// 设置起始时间
void T1005::setStartTime(const std::string& time) {
    startTime = time;
}

// 获取结束时间
std::string T1005::getEndTime() const {
    return endTime;
}

// 设置结束时间
void T1005::setEndTime(const std::string& time) {
    endTime = time;
}

// 获取上车人数
uint16_t T1005::getGetOnCount() const {
    return getOnCount;
}

// 设置上车人数
void T1005::setGetOnCount(uint16_t count) {
    getOnCount = count;
}

// 获取下车人数
uint16_t T1005::getGetOffCount() const {
    return getOffCount;
}

// 设置下车人数
void T1005::setGetOffCount(uint16_t count) {
    getOffCount = count;
}
