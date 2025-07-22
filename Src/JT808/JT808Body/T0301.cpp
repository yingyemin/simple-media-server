#include "T0301.h"

// 默认构造函数
T0301::T0301() = default;
// 析构函数
T0301::~T0301() = default;

// 获取事件 ID
int T0301::getEventId() const {
    return eventId;
}

// 设置事件 ID，支持链式调用
T0301& T0301::setEventId(int id) {
    eventId = id;
    return *this;
}
