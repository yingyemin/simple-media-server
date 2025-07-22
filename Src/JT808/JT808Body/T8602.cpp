#include "T8602.h"
#include <cmath>

// Rectangle 类构造函数
T8602::Rectangle::Rectangle() : id(0), attribute(0), latitudeUL(0), longitudeUL(0),
                                latitudeLR(0), longitudeLR(0), name("") {}

T8602::Rectangle::Rectangle(int id, int attribute, int latitudeUL, int longitudeUL, int latitudeLR, int longitudeLR,
                            const std::string& startTime, const std::string& endTime,
                            const std::shared_ptr<int>& maxSpeed, const std::shared_ptr<int>& duration)
    : id(id), attribute(attribute), latitudeUL(latitudeUL), longitudeUL(longitudeUL),
      latitudeLR(latitudeLR), longitudeLR(longitudeLR),
      startTime(startTime), endTime(endTime), maxSpeed(maxSpeed), duration(duration), name("") {
    setStartTime(startTime);
    setEndTime(endTime);
    setMaxSpeed(maxSpeed);
    setDuration(duration);
}

T8602::Rectangle::Rectangle(int id, int attribute, int latitudeUL, int longitudeUL, int latitudeLR, int longitudeLR,
                            const std::string& startTime, const std::string& endTime,
                            const std::shared_ptr<int>& maxSpeed, const std::shared_ptr<int>& duration,
                            const std::shared_ptr<int>& nightMaxSpeed, const std::string& name)
    : Rectangle(id, attribute, latitudeUL, longitudeUL, latitudeLR, longitudeLR,
                startTime, endTime, maxSpeed, duration) {
    setNightMaxSpeed(nightMaxSpeed);
    this->name = name;
}

// Rectangle 类 Getter 方法
int T8602::Rectangle::getId() const {
    return id;
}

int T8602::Rectangle::getAttribute() const {
    return attribute;
}

int T8602::Rectangle::getLatitudeUL() const {
    return latitudeUL;
}

int T8602::Rectangle::getLongitudeUL() const {
    return longitudeUL;
}

int T8602::Rectangle::getLatitudeLR() const {
    return latitudeLR;
}

int T8602::Rectangle::getLongitudeLR() const {
    return longitudeLR;
}

std::string T8602::Rectangle::getStartTime() const {
    return startTime;
}

std::string T8602::Rectangle::getEndTime() const {
    return endTime;
}

std::shared_ptr<int> T8602::Rectangle::getMaxSpeed() const {
    return maxSpeed;
}

std::shared_ptr<int> T8602::Rectangle::getDuration() const {
    return duration;
}

std::shared_ptr<int> T8602::Rectangle::getNightMaxSpeed() const {
    return nightMaxSpeed;
}

std::string T8602::Rectangle::getName() const {
    return name;
}

// Rectangle 类 Setter 方法
T8602::Rectangle& T8602::Rectangle::setId(int newId) {
    id = newId;
    return *this;
}

T8602::Rectangle& T8602::Rectangle::setAttribute(int newAttribute) {
    attribute = newAttribute;
    return *this;
}

T8602::Rectangle& T8602::Rectangle::setLatitudeUL(int newLatitudeUL) {
    latitudeUL = newLatitudeUL;
    return *this;
}

T8602::Rectangle& T8602::Rectangle::setLongitudeUL(int newLongitudeUL) {
    longitudeUL = newLongitudeUL;
    return *this;
}

T8602::Rectangle& T8602::Rectangle::setLatitudeLR(int newLatitudeLR) {
    latitudeLR = newLatitudeLR;
    return *this;
}

T8602::Rectangle& T8602::Rectangle::setLongitudeLR(int newLongitudeLR) {
    longitudeLR = newLongitudeLR;
    return *this;
}

T8602::Rectangle& T8602::Rectangle::setStartTime(const std::string& newStartTime) {
    attribute = (attribute & ~1) | (newStartTime.empty() ? 0 : 1);
    startTime = newStartTime;
    return *this;
}

T8602::Rectangle& T8602::Rectangle::setEndTime(const std::string& newEndTime) {
    attribute = (attribute & ~1) | (newEndTime.empty() ? 0 : 1);
    endTime = newEndTime;
    return *this;
}

T8602::Rectangle& T8602::Rectangle::setMaxSpeed(const std::shared_ptr<int>& newMaxSpeed) {
    attribute = (attribute & ~(1 << 1)) | (newMaxSpeed ? (1 << 1) : 0);
    maxSpeed = newMaxSpeed;
    return *this;
}

T8602::Rectangle& T8602::Rectangle::setDuration(const std::shared_ptr<int>& newDuration) {
    attribute = (attribute & ~(1 << 1)) | (newDuration ? (1 << 1) : 0);
    duration = newDuration;
    return *this;
}

T8602::Rectangle& T8602::Rectangle::setNightMaxSpeed(const std::shared_ptr<int>& newNightMaxSpeed) {
    attribute = (attribute & ~(1 << 1)) | (newNightMaxSpeed ? (1 << 1) : 0);
    nightMaxSpeed = newNightMaxSpeed;
    return *this;
}

T8602::Rectangle& T8602::Rectangle::setName(const std::string& newName) {
    name = newName;
    return *this;
}

bool T8602::Rectangle::attrBit(int i) const {
    return (attribute & (1 << i)) != 0;
}

// T8602 类构造函数
T8602::T8602() : action(0) {}

// T8602 类 Getter 方法
int T8602::getAction() const {
    return action;
}

const std::vector<T8602::Rectangle>& T8602::getItems() const {
    return items;
}

// T8602 类 Setter 方法
T8602& T8602::setAction(int newAction) {
    action = newAction;
    return *this;
}

T8602& T8602::setItems(const std::vector<Rectangle>& newItems) {
    items = newItems;
    return *this;
}

T8602& T8602::addItem(const Rectangle& item) {
    items.push_back(item);
    return *this;
}
