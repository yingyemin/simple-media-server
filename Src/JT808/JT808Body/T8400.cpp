#include "T8400.h"

// 默认构造函数
T8400::T8400() : type(0), phoneNumber("") {}

// 析构函数
T8400::~T8400() = default;

// 获取静态常量值的方法
int T8400::getNormal() {
    return Normal;
}

int T8400::getListen() {
    return Listen;
}

// Getter 方法实现
int T8400::getType() const {
    return type;
}

const std::string& T8400::getPhoneNumber() const {
    return phoneNumber;
}

// Setter 方法实现，支持链式调用
T8400& T8400::setType(int newType) {
    type = newType;
    return *this;
}

T8400& T8400::setPhoneNumber(const std::string& newPhoneNumber) {
    phoneNumber = newPhoneNumber;
    return *this;
}
