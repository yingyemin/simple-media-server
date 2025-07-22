#include "T8805.h"

// 默认构造函数
T8805::T8805() : id(0), deleteFlag(0) {}

// Getter 方法实现
int T8805::getId() const {
    return id;
}

int T8805::getDeleteFlag() const {
    return deleteFlag;
}

// Setter 方法实现，支持链式调用
T8805& T8805::setId(int newId) {
    id = newId;
    return *this;
}

T8805& T8805::setDeleteFlag(int newDeleteFlag) {
    deleteFlag = newDeleteFlag;
    return *this;
}
