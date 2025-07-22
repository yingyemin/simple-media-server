#include "T8600.h"
#include <cmath>

// Circle 类构造函数
T8600::Circle::Circle() : id(0), attribute(0), latitude(0), longitude(0), radius(0), name("") {}

T8600::Circle::Circle(int id, int attribute, int latitude, int longitude, int radius, const std::string& startTime, const std::string& endTime, 
                      const std::shared_ptr<int>& maxSpeed, const std::shared_ptr<int>& duration)
    : id(id), attribute(attribute), latitude(latitude), longitude(longitude), radius(radius), 
      startTime(startTime), endTime(endTime), maxSpeed(maxSpeed), duration(duration), name("") {
    setStartTime(startTime);
    setEndTime(endTime);
    setMaxSpeed(maxSpeed);
    setDuration(duration);
}

T8600::Circle::Circle(int id, int attribute, int latitude, int longitude, int radius, const std::string& startTime, const std::string& endTime, 
                      const std::shared_ptr<int>& maxSpeed, const std::shared_ptr<int>& duration, const std::shared_ptr<int>& nightMaxSpeed, const std::string& name)
    : Circle(id, attribute, latitude, longitude, radius, startTime, endTime, maxSpeed, duration) {
    setNightMaxSpeed(nightMaxSpeed);
    this->name = name;
}

// Circle 类析构函数
T8600::Circle::~Circle() = default;

// Circle 类 Getter 方法
int T8600::Circle::getId() const {
    return id;
}

int T8600::Circle::getAttribute() const {
    return attribute;
}

int T8600::Circle::getLatitude() const {
    return latitude;
}

int T8600::Circle::getLongitude() const {
    return longitude;
}

int T8600::Circle::getRadius() const {
    return radius;
}

std::string T8600::Circle::getStartTime() const {
    return startTime;
}

std::string T8600::Circle::getEndTime() const {
    return endTime;
}

std::shared_ptr<int> T8600::Circle::getMaxSpeed() const {
    return maxSpeed;
}

std::shared_ptr<int> T8600::Circle::getDuration() const {
    return duration;
}

std::shared_ptr<int> T8600::Circle::getNightMaxSpeed() const {
    return nightMaxSpeed;
}

std::string T8600::Circle::getName() const {
    return name;
}

// Circle 类 Setter 方法
T8600::Circle& T8600::Circle::setId(int newId) {
    id = newId;
    return *this;
}

T8600::Circle& T8600::Circle::setAttribute(int newAttribute) {
    attribute = newAttribute;
    return *this;
}

T8600::Circle& T8600::Circle::setLatitude(int newLatitude) {
    latitude = newLatitude;
    return *this;
}

T8600::Circle& T8600::Circle::setLongitude(int newLongitude) {
    longitude = newLongitude;
    return *this;
}

T8600::Circle& T8600::Circle::setRadius(int newRadius) {
    radius = newRadius;
    return *this;
}

T8600::Circle& T8600::Circle::setStartTime(const std::string& newStartTime) {
    attribute = (attribute & ~1) | (newStartTime.empty() ? 0 : 1);
    startTime = newStartTime;
    return *this;
}

T8600::Circle& T8600::Circle::setEndTime(const std::string& newEndTime) {
    attribute = (attribute & ~1) | (newEndTime.empty() ? 0 : 1);
    endTime = newEndTime;
    return *this;
}

T8600::Circle& T8600::Circle::setMaxSpeed(const std::shared_ptr<int>& newMaxSpeed) {
    attribute = (attribute & ~(1 << 1)) | (newMaxSpeed ? (1 << 1) : 0);
    maxSpeed = newMaxSpeed;
    return *this;
}

T8600::Circle& T8600::Circle::setDuration(const std::shared_ptr<int>& newDuration) {
    attribute = (attribute & ~(1 << 1)) | (newDuration ? (1 << 1) : 0);
    duration = newDuration;
    return *this;
}

T8600::Circle& T8600::Circle::setNightMaxSpeed(const std::shared_ptr<int>& newNightMaxSpeed) {
    attribute = (attribute & ~(1 << 1)) | (newNightMaxSpeed ? (1 << 1) : 0);
    nightMaxSpeed = newNightMaxSpeed;
    return *this;
}

T8600::Circle& T8600::Circle::setName(const std::string& newName) {
    name = newName;
    return *this;
}

bool T8600::Circle::attrBit(int i) const {
    return (attribute & (1 << i)) != 0;
}

// T8600 类构造函数
T8600::T8600() : action(0) {}

// T8600 类析构函数
T8600::~T8600() = default;

// T8600 类 Getter 方法
int T8600::getAction() const {
    return action;
}

const std::vector<T8600::Circle>& T8600::getItems() const {
    return items;
}

// T8600 类 Setter 方法
T8600& T8600::setAction(int newAction) {
    action = newAction;
    return *this;
}

T8600& T8600::setItems(const std::vector<Circle>& newItems) {
    items = newItems;
    return *this;
}

T8600& T8600::addItem(const Circle& item) {
    items.push_back(item);
    return *this;
}
