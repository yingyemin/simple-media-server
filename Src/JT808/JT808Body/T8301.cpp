#include "T8301.h"

// Event 类默认构造函数
T8301::Event::Event() : id(0), content("") {}

// Event 类析构函数
T8301::Event::~Event() = default;

// Event 类 Getter 方法实现
int T8301::Event::getId() const {
    return id;
}

const std::string& T8301::Event::getContent() const {
    return content;
}

// Event 类 Setter 方法实现，支持链式调用
T8301::Event& T8301::Event::setId(int newId) {
    id = newId;
    return *this;
}

T8301::Event& T8301::Event::setContent(const std::string& newContent) {
    content = newContent;
    return *this;
}

// T8301 类默认构造函数
T8301::T8301() : type(0) {}

// T8301 类析构函数
T8301::~T8301() = default;

// T8301 类 Getter 方法实现
int T8301::getType() const {
    return type;
}

const std::vector<T8301::Event>& T8301::getEvents() const {
    return events;
}

// T8301 类 Setter 方法实现，支持链式调用
T8301& T8301::setType(int newType) {
    type = newType;
    return *this;
}

T8301& T8301::setEvents(const std::vector<Event>& newEvents) {
    events = newEvents;
    return *this;
}

// T8301 类添加事件方法实现
T8301& T8301::addEvent(int id, const std::string& content) {
    Event event;
    event.setId(id);
    event.setContent(content);
    events.emplace_back(event);
    return *this;
}
