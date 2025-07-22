#include "T8108.h"

// 默认构造函数
T8108::T8108() : type(0), makerId(""), version("") {}

// 析构函数
T8108::~T8108() = default;

// 获取静态常量值的方法
int T8108::getTerminal() {
    return Terminal;
}

int T8108::getCardReader() {
    return CardReader;
}

int T8108::getBeidou() {
    return Beidou;
}

// Getter 方法实现
int T8108::getType() const {
    return type;
}

const std::string& T8108::getMakerId() const {
    return makerId;
}

const std::string& T8108::getVersion() const {
    return version;
}

const std::vector<uint8_t>& T8108::getPacket() const {
    return packet;
}

// Setter 方法实现，支持链式调用
T8108& T8108::setType(int newType) {
    type = newType;
    return *this;
}

T8108& T8108::setMakerId(const std::string& newMakerId) {
    makerId = newMakerId;
    return *this;
}

T8108& T8108::setVersion(const std::string& newVersion) {
    version = newVersion;
    return *this;
}

T8108& T8108::setPacket(const std::vector<uint8_t>& newPacket) {
    packet = newPacket;
    return *this;
}
