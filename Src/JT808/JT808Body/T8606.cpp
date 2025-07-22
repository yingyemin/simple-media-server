#include "T8606.h"

// Line 类构造函数
T8606::Line::Line() : id(0), routeId(0), latitude(0), longitude(0), width(0), attribute(0) {}

T8606::Line::Line(int id, int routeId, int latitude, int longitude, int width, int attribute,
                  const std::shared_ptr<int>& upperLimit, const std::shared_ptr<int>& lowerLimit,
                  const std::shared_ptr<int>& maxSpeed, const std::shared_ptr<int>& duration)
    : id(id), routeId(routeId), latitude(latitude), longitude(longitude), width(width), attribute(attribute) {
    setUpperLimit(upperLimit);
    setLowerLimit(lowerLimit);
    setMaxSpeed(maxSpeed);
    setDuration(duration);
}

T8606::Line::Line(int id, int routeId, int latitude, int longitude, int width, int attribute,
                  const std::shared_ptr<int>& upperLimit, const std::shared_ptr<int>& lowerLimit,
                  const std::shared_ptr<int>& maxSpeed, const std::shared_ptr<int>& duration,
                  const std::shared_ptr<int>& nightMaxSpeed)
    : Line(id, routeId, latitude, longitude, width, attribute, upperLimit, lowerLimit, maxSpeed, duration) {
    setNightMaxSpeed(nightMaxSpeed);
}

// Line 类 Getter 方法
int T8606::Line::getId() const {
    return id;
}

int T8606::Line::getRouteId() const {
    return routeId;
}

int T8606::Line::getLatitude() const {
    return latitude;
}

int T8606::Line::getLongitude() const {
    return longitude;
}

int T8606::Line::getWidth() const {
    return width;
}

int T8606::Line::getAttribute() const {
    return attribute;
}

std::shared_ptr<int> T8606::Line::getUpperLimit() const {
    return upperLimit;
}

std::shared_ptr<int> T8606::Line::getLowerLimit() const {
    return lowerLimit;
}

std::shared_ptr<int> T8606::Line::getMaxSpeed() const {
    return maxSpeed;
}

std::shared_ptr<int> T8606::Line::getDuration() const {
    return duration;
}

std::shared_ptr<int> T8606::Line::getNightMaxSpeed() const {
    return nightMaxSpeed;
}

// Line 类 Setter 方法
T8606::Line& T8606::Line::setId(int newId) {
    id = newId;
    return *this;
}

T8606::Line& T8606::Line::setRouteId(int newRouteId) {
    routeId = newRouteId;
    return *this;
}

T8606::Line& T8606::Line::setLatitude(int newLatitude) {
    latitude = newLatitude;
    return *this;
}

T8606::Line& T8606::Line::setLongitude(int newLongitude) {
    longitude = newLongitude;
    return *this;
}

T8606::Line& T8606::Line::setWidth(int newWidth) {
    width = newWidth;
    return *this;
}

T8606::Line& T8606::Line::setAttribute(int newAttribute) {
    attribute = newAttribute;
    return *this;
}

T8606::Line& T8606::Line::setUpperLimit(const std::shared_ptr<int>& newUpperLimit) {
    attribute = (attribute & ~1) | (newUpperLimit ? 1 : 0);
    upperLimit = newUpperLimit;
    return *this;
}

T8606::Line& T8606::Line::setLowerLimit(const std::shared_ptr<int>& newLowerLimit) {
    attribute = (attribute & ~1) | (newLowerLimit ? 1 : 0);
    lowerLimit = newLowerLimit;
    return *this;
}

T8606::Line& T8606::Line::setMaxSpeed(const std::shared_ptr<int>& newMaxSpeed) {
    attribute = (attribute & ~(1 << 1)) | (newMaxSpeed ? (1 << 1) : 0);
    maxSpeed = newMaxSpeed;
    return *this;
}

T8606::Line& T8606::Line::setDuration(const std::shared_ptr<int>& newDuration) {
    attribute = (attribute & ~(1 << 1)) | (newDuration ? (1 << 1) : 0);
    duration = newDuration;
    return *this;
}

T8606::Line& T8606::Line::setNightMaxSpeed(const std::shared_ptr<int>& newNightMaxSpeed) {
    attribute = (attribute & ~(1 << 1)) | (newNightMaxSpeed ? (1 << 1) : 0);
    nightMaxSpeed = newNightMaxSpeed;
    return *this;
}

bool T8606::Line::attrBit(int i) const {
    return (attribute & (1 << i)) != 0;
}

// T8606 类构造函数
T8606::T8606() : id(0), attribute(0) {}

// T8606 类 Getter 方法
int T8606::getId() const {
    return id;
}

int T8606::getAttribute() const {
    return attribute;
}

std::string T8606::getStartTime() const {
    return startTime;
}

std::string T8606::getEndTime() const {
    return endTime;
}

const std::vector<T8606::Line>& T8606::getItems() const {
    return items;
}

std::string T8606::getName() const {
    return name;
}

// T8606 类 Setter 方法
T8606& T8606::setId(int newId) {
    id = newId;
    return *this;
}

T8606& T8606::setAttribute(int newAttribute) {
    attribute = newAttribute;
    return *this;
}

T8606& T8606::setStartTime(const std::string& newStartTime) {
    attribute = (attribute & ~1) | (newStartTime.empty() ? 0 : 1);
    startTime = newStartTime;
    return *this;
}

T8606& T8606::setEndTime(const std::string& newEndTime) {
    attribute = (attribute & ~1) | (newEndTime.empty() ? 0 : 1);
    endTime = newEndTime;
    return *this;
}

T8606& T8606::setItems(const std::vector<Line>& newItems) {
    items = newItems;
    return *this;
}

T8606& T8606::setName(const std::string& newName) {
    name = newName;
    return *this;
}

// T8606 类添加拐点方法
T8606& T8606::addItem(const Line& item) {
    items.push_back(item);
    return *this;
}

bool T8606::attrBit(int i) const {
    return (attribute & (1 << i)) != 0;
}
