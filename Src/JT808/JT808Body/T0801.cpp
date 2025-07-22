#include "T0801.h"

// 默认构造函数
T0801::T0801() : id(0), type(0), format(0), event(0), channelId(0) {}

// 析构函数
T0801::~T0801() = default;

// Getter 方法实现
int T0801::getId() const {
    return id;
}

int T0801::getType() const {
    return type;
}

int T0801::getFormat() const {
    return format;
}

int T0801::getEvent() const {
    return event;
}

int T0801::getChannelId() const {
    return channelId;
}

const T0200_VersionMinus1And0* T0801::getLocation() const {
    return location.get();
}

const T0200_Version1* T0801::getLocation2019() const {
    return location2019.get();
}

const ByteBuf* T0801::getPacket() const {
    return packet.get();
}

// Setter 方法实现，支持链式调用
T0801& T0801::setId(int newId) {
    id = newId;
    return *this;
}

T0801& T0801::setType(int newType) {
    type = newType;
    return *this;
}

T0801& T0801::setFormat(int newFormat) {
    format = newFormat;
    return *this;
}

T0801& T0801::setEvent(int newEvent) {
    event = newEvent;
    return *this;
}

T0801& T0801::setChannelId(int newChannelId) {
    channelId = newChannelId;
    return *this;
}

T0801& T0801::setLocation(std::unique_ptr<T0200_VersionMinus1And0> newLocation) {
    location = std::move(newLocation);
    return *this;
}

T0801& T0801::setLocation(std::unique_ptr<T0200_Version1> newLocation) {
    location2019 = std::move(newLocation);
    return *this;
}

T0801& T0801::setPacket(std::unique_ptr<ByteBuf> newPacket) {
    packet = std::move(newPacket);
    return *this;
}

// noBuffer 方法实现
bool T0801::noBuffer() const {
    return packet == nullptr;
}
