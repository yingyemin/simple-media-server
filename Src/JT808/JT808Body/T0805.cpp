#include "T0805.h"

// 默认构造函数
T0805::T0805() : responseSerialNo(0), result(0) {}

// 析构函数
T0805::~T0805() = default;

// Getter 方法实现
int T0805::getResponseSerialNo() const {
    return responseSerialNo;
}

int T0805::getResult() const {
    return result;
}

const std::vector<int>& T0805::getId() const {
    return id;
}

// Setter 方法实现，支持链式调用
T0805& T0805::setResponseSerialNo(int newResponseSerialNo) {
    responseSerialNo = newResponseSerialNo;
    return *this;
}

T0805& T0805::setResult(int newResult) {
    result = newResult;
    return *this;
}

T0805& T0805::setId(const std::vector<int>& newId) {
    id = newId;
    return *this;
}

T0805& T0805::addId(int newId) {
    id.push_back(newId);
    return *this;
}
