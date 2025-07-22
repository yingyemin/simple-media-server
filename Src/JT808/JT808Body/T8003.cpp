#include "T8003.h"

// T8003_Version_1_0 默认构造函数
T8003_Version_1_0::T8003_Version_1_0() : responseSerialNo(0) {}

// T8003_Version_1_0 析构函数
T8003_Version_1_0::~T8003_Version_1_0() = default;

// T8003_Version_1_0 Getter 方法实现
int T8003_Version_1_0::getResponseSerialNo() const {
    return responseSerialNo;
}

const std::vector<short>& T8003_Version_1_0::getId() const {
    return id;
}

// T8003_Version_1_0 Setter 方法实现，支持链式调用
T8003_Version_1_0& T8003_Version_1_0::setResponseSerialNo(int newResponseSerialNo) {
    responseSerialNo = newResponseSerialNo;
    return *this;
}

T8003_Version_1_0& T8003_Version_1_0::setId(const std::vector<short>& newId) {
    id = newId;
    return *this;
}

T8003_Version_1_0& T8003_Version_1_0::addId(short newId) {
    id.push_back(newId);
    return *this;
}

// T8003_Version_1 默认构造函数
T8003_Version_1::T8003_Version_1() : responseSerialNo(0) {}

// T8003_Version_1 析构函数
T8003_Version_1::~T8003_Version_1() = default;

// T8003_Version_1 Getter 方法实现
int T8003_Version_1::getResponseSerialNo() const {
    return responseSerialNo;
}

const std::vector<short>& T8003_Version_1::getId() const {
    return id;
}

// T8003_Version_1 Setter 方法实现，支持链式调用
T8003_Version_1& T8003_Version_1::setResponseSerialNo(int newResponseSerialNo) {
    responseSerialNo = newResponseSerialNo;
    return *this;
}

T8003_Version_1& T8003_Version_1::setId(const std::vector<short>& newId) {
    id = newId;
    return *this;
}

T8003_Version_1& T8003_Version_1::addId(short newId) {
    id.push_back(newId);
    return *this;
}
