#include "T0800.h"

// 默认构造函数
T0800::T0800() : id(0), type(0), format(0), event(0), channelId(0) {}

// 析构函数
T0800::~T0800() = default;

// Getter 方法实现
int T0800::getId() const {
    return id;
}

int T0800::getType() const {
    return type;
}

int T0800::getFormat() const {
    return format;
}

int T0800::getEvent() const {
    return event;
}

int T0800::getChannelId() const {
    return channelId;
}

// Setter 方法实现，支持链式调用
T0800& T0800::setId(int newId) {
    id = newId;
    return *this;
}

T0800& T0800::setType(int newType) {
    type = newType;
    return *this;
}

T0800& T0800::setFormat(int newFormat) {
    format = newFormat;
    return *this;
}

T0800& T0800::setEvent(int newEvent) {
    event = newEvent;
    return *this;
}

T0800& T0800::setChannelId(int newChannelId) {
    channelId = newChannelId;
    return *this;
}
