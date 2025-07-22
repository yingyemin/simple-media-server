#include "T8303.h"

// Info 类默认构造函数
T8303::Info::Info() : type(0), name("") {}

// Info 类析构函数
T8303::Info::~Info() = default;

// Info 类 Getter 方法实现
int T8303::Info::getType() const {
    return type;
}

const std::string& T8303::Info::getName() const {
    return name;
}

// Info 类 Setter 方法实现，支持链式调用
T8303::Info& T8303::Info::setType(int newType) {
    type = newType;
    return *this;
}

T8303::Info& T8303::Info::setName(const std::string& newName) {
    name = newName;
    return *this;
}

// T8303 类默认构造函数
T8303::T8303() : type(0) {}

// T8303 类析构函数
T8303::~T8303() = default;

// T8303 类 Getter 方法实现
int T8303::getType() const {
    return type;
}

const std::vector<T8303::Info>& T8303::getInfos() const {
    return infos;
}

// T8303 类 Setter 方法实现，支持链式调用
T8303& T8303::setType(int newType) {
    type = newType;
    return *this;
}

T8303& T8303::setInfos(const std::vector<Info>& newInfos) {
    infos = newInfos;
    return *this;
}

// T8303 类添加信息项方法实现
T8303& T8303::addInfo(int type, const std::string& name) {
    Info info;
    info.setType(type);
    info.setName(name);
    infos.emplace_back(info);
    return *this;
}
