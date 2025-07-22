#include "T8302.h"

// Option 类默认构造函数
T8302::Option::Option() : id(0), content("") {}

// Option 类析构函数
T8302::Option::~Option() = default;

// Option 类 Getter 方法实现
int T8302::Option::getId() const {
    return id;
}

const std::string& T8302::Option::getContent() const {
    return content;
}

// Option 类 Setter 方法实现，支持链式调用
T8302::Option& T8302::Option::setId(int newId) {
    id = newId;
    return *this;
}

T8302::Option& T8302::Option::setContent(const std::string& newContent) {
    content = newContent;
    return *this;
}

// T8302 类默认构造函数
T8302::T8302() : sign(0), content("") {}

// T8302 类析构函数
T8302::~T8302() = default;

// T8302 类 Getter 方法实现
int T8302::getSign() const {
    return sign;
}

const std::string& T8302::getContent() const {
    return content;
}

const std::vector<T8302::Option>& T8302::getOptions() const {
    return options;
}

// T8302 类 Setter 方法实现，支持链式调用
T8302& T8302::setSign(int newSign) {
    sign = newSign;
    return *this;
}

T8302& T8302::setContent(const std::string& newContent) {
    content = newContent;
    return *this;
}

T8302& T8302::setOptions(const std::vector<Option>& newOptions) {
    options = newOptions;
    return *this;
}

// T8302 类添加选项方法实现
T8302& T8302::addOption(int id, const std::string& content) {
    Option option;
    option.setId(id);
    option.setContent(content);
    options.emplace_back(option);
    return *this;
}
