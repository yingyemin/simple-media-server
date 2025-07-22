#include "T8803.h"

// 默认构造函数
T8803::T8803() : type(0), channelId(0), event(0), startTime(""), endTime(""), deleteFlag(0) {}

// Getter 方法实现
int T8803::getType() const {
    return type;
}

int T8803::getChannelId() const {
    return channelId;
}

int T8803::getEvent() const {
    return event;
}

std::string T8803::getStartTime() const {
    return startTime;
}

std::string T8803::getEndTime() const {
    return endTime;
}

int T8803::getDeleteFlag() const {
    return deleteFlag;
}

// Setter 方法实现，支持链式调用
T8803& T8803::setType(int newType) {
    type = newType;
    return *this;
}

T8803& T8803::setChannelId(int newChannelId) {
    channelId = newChannelId;
    return *this;
}

T8803& T8803::setEvent(int newEvent) {
    event = newEvent;
    return *this;
}

T8803& T8803::setStartTime(const std::string& newStartTime) {
    startTime = newStartTime;
    return *this;
}

T8803& T8803::setEndTime(const std::string& newEndTime) {
    endTime = newEndTime;
    return *this;
}

T8803& T8803::setDeleteFlag(int newDeleteFlag) {
    deleteFlag = newDeleteFlag;
    return *this;
}
