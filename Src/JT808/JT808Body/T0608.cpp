#include "T0608.h"
#include <cstring>

// 默认构造函数
T0608::T0608() : type(0), bytes(nullptr), bytesLength(0) {}

// 析构函数，释放动态分配的内存
T0608::~T0608() {
    if (bytes) {
        delete[] bytes;
    }
}

// 获取查询类型
int T0608::getType() const {
    return type;
}

// 设置查询类型，支持链式调用
T0608& T0608::setType(int t) {
    type = t;
    return *this;
}

// 获取查询返回的数据
const unsigned char* T0608::getBytes() const {
    return bytes;
}

// 设置查询返回的数据，支持链式调用
T0608& T0608::setBytes(const unsigned char* data, int length) {
    if (bytes) {
        delete[] bytes;
    }
    bytesLength = length;
    bytes = new unsigned char[length];
    std::memcpy(bytes, data, length);
    return *this;
}

// 获取数据长度
int T0608::getBytesLength() const {
    return bytesLength;
}
