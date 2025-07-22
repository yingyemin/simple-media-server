#include "T8802.h"

// 默认构造函数
T8802::T8802() : type(0), channelId(0), event(0), startTime(""), endTime("") {}

// Getter 方法实现
int T8802::getType() const {
    return type;
}

int T8802::getChannelId() const {
    return channelId;
}

int T8802::getEvent() const {
    return event;
}

std::string T8802::getStartTime() const {
    return startTime;
}

std::string T8802::getEndTime() const {
    return endTime;
}

// Setter 方法实现，支持链式调用
T8802& T8802::setType(int newType) {
    type = newType;
    return *this;
}

T8802& T8802::setChannelId(int newChannelId) {
    channelId = newChannelId;
    return *this;
}

T8802& T8802::setEvent(int newEvent) {
    event = newEvent;
    return *this;
}

T8802& T8802::setStartTime(const std::string& newStartTime) {
    startTime = newStartTime;
    return *this;
}

T8802& T8802::setEndTime(const std::string& newEndTime) {
    endTime = newEndTime;
    return *this;
}
