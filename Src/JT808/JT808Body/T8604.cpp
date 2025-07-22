#include "T8604.h"

// Point 类构造函数
T8604::Point::Point() : latitude(0), longitude(0) {}

// Point 类 Getter 方法
int T8604::Point::getLatitude() const {
    return latitude;
}

int T8604::Point::getLongitude() const {
    return longitude;
}

// Point 类 Setter 方法
T8604::Point& T8604::Point::setLatitude(int newLatitude) {
    latitude = newLatitude;
    return *this;
}

T8604::Point& T8604::Point::setLongitude(int newLongitude) {
    longitude = newLongitude;
    return *this;
}

// T8604 类构造函数
T8604::T8604() : id(0), attribute(0) {}

// T8604 类 Getter 方法
int T8604::getId() const {
    return id;
}

int T8604::getAttribute() const {
    return attribute;
}

std::string T8604::getStartTime() const {
    return startTime;
}

std::string T8604::getEndTime() const {
    return endTime;
}

std::shared_ptr<int> T8604::getMaxSpeed() const {
    return maxSpeed;
}

std::shared_ptr<int> T8604::getDuration() const {
    return duration;
}

const std::vector<T8604::Point>& T8604::getPoints() const {
    return points;
}

std::shared_ptr<int> T8604::getNightMaxSpeed() const {
    return nightMaxSpeed;
}

std::string T8604::getName() const {
    return name;
}

// T8604 类 Setter 方法
T8604& T8604::setId(int newId) {
    id = newId;
    return *this;
}

T8604& T8604::setAttribute(int newAttribute) {
    attribute = newAttribute;
    return *this;
}

T8604& T8604::setStartTime(const std::string& newStartTime) {
    attribute = (attribute & ~1) | (newStartTime.empty() ? 0 : 1);
    startTime = newStartTime;
    return *this;
}

T8604& T8604::setEndTime(const std::string& newEndTime) {
    attribute = (attribute & ~1) | (newEndTime.empty() ? 0 : 1);
    endTime = newEndTime;
    return *this;
}

T8604& T8604::setMaxSpeed(const std::shared_ptr<int>& newMaxSpeed) {
    attribute = (attribute & ~(1 << 1)) | (newMaxSpeed ? (1 << 1) : 0);
    maxSpeed = newMaxSpeed;
    return *this;
}

T8604& T8604::setDuration(const std::shared_ptr<int>& newDuration) {
    attribute = (attribute & ~(1 << 1)) | (newDuration ? (1 << 1) : 0);
    duration = newDuration;
    return *this;
}

T8604& T8604::setPoints(const std::vector<Point>& newPoints) {
    points = newPoints;
    return *this;
}

T8604& T8604::setNightMaxSpeed(const std::shared_ptr<int>& newNightMaxSpeed) {
    attribute = (attribute & ~(1 << 1)) | (newNightMaxSpeed ? (1 << 1) : 0);
    nightMaxSpeed = newNightMaxSpeed;
    return *this;
}

T8604& T8604::setName(const std::string& newName) {
    name = newName;
    return *this;
}

// T8604 类添加顶点方法
T8604& T8604::addPoint(int longitude, int latitude) {
    Point point;
    point.setLatitude(latitude);
    point.setLongitude(longitude);
    points.emplace_back(point);
    return *this;
}

// T8604 类判断属性位方法
bool T8604::attrBit(int i) const {
    return (attribute & (1 << i)) != 0;
}
