#include "T0303.h"

// 默认构造函数
T0303::T0303() = default;
// 析构函数
T0303::~T0303() = default;

// 获取消息类型
int T0303::getType() const {
    return type;
}

// 设置消息类型，支持链式调用
T0303& T0303::setType(int t) {
    type = t;
    return *this;
}

// 获取点播/取消标志
int T0303::getAction() const {
    return action;
}

// 设置点播/取消标志，支持链式调用
T0303& T0303::setAction(int a) {
    action = a;
    return *this;
}
