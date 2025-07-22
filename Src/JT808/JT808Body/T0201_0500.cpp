#include "T0201_0500.h"
#include <stdexcept>

// T0201_0500_VersionMinus1And0 类方法实现
int T0201_0500_VersionMinus1And0::getWarnBit() const {
    return warnBit;
}

T0201_0500_VersionMinus1And0& T0201_0500_VersionMinus1And0::setWarnBit(int bit) {
    warnBit = bit;
    return *this;
}

int T0201_0500_VersionMinus1And0::getStatusBit() const {
    return statusBit;
}

T0201_0500_VersionMinus1And0& T0201_0500_VersionMinus1And0::setStatusBit(int bit) {
    statusBit = bit;
    return *this;
}

int T0201_0500_VersionMinus1And0::getLatitude() const {
    return latitude;
}

T0201_0500_VersionMinus1And0& T0201_0500_VersionMinus1And0::setLatitude(int lat) {
    latitude = lat;
    return *this;
}

int T0201_0500_VersionMinus1And0::getLongitude() const {
    return longitude;
}

T0201_0500_VersionMinus1And0& T0201_0500_VersionMinus1And0::setLongitude(int lon) {
    longitude = lon;
    return *this;
}

int T0201_0500_VersionMinus1And0::getAltitude() const {
    return altitude;
}

T0201_0500_VersionMinus1And0& T0201_0500_VersionMinus1And0::setAltitude(int alt) {
    altitude = alt;
    return *this;
}

int T0201_0500_VersionMinus1And0::getSpeed() const {
    return speed;
}

T0201_0500_VersionMinus1And0& T0201_0500_VersionMinus1And0::setSpeed(int spd) {
    speed = spd;
    return *this;
}

int T0201_0500_VersionMinus1And0::getDirection() const {
    return direction;
}

T0201_0500_VersionMinus1And0& T0201_0500_VersionMinus1And0::setDirection(int dir) {
    direction = dir;
    return *this;
}

LocalDateTime T0201_0500_VersionMinus1And0::getDeviceTime() const {
    return deviceTime;
}

T0201_0500_VersionMinus1And0& T0201_0500_VersionMinus1And0::setDeviceTime(LocalDateTime time) {
    deviceTime = time;
    return *this;
}

std::map<int, std::shared_ptr<void>> T0201_0500_VersionMinus1And0::getAttributes() const {
    return attributes;
}

T0201_0500_VersionMinus1And0& T0201_0500_VersionMinus1And0::setAttributes(const std::map<int, std::shared_ptr<void>>& attrs) {
    attributes = attrs;
    return *this;
}

int T0201_0500_VersionMinus1And0::getAttributeInt(int key) const {
    auto it = attributes.find(key);
    if (it != attributes.end() && it->second) {
        return *static_cast<int*>(it->second.get());
    }
    return 0;
}

long T0201_0500_VersionMinus1And0::getAttributeLong(int key) const {
    auto it = attributes.find(key);
    if (it != attributes.end() && it->second) {
        return *static_cast<long*>(it->second.get());
    }
    return 0L;
}

double T0201_0500_VersionMinus1And0::getLng() const {
    return longitude / 1000000.0;
}

double T0201_0500_VersionMinus1And0::getLat() const {
    return latitude / 1000000.0;
}

float T0201_0500_VersionMinus1And0::getSpeedKph() const {
    return speed / 10.0f;
}

int T0201_0500_VersionMinus1And0::getResponseSerialNo() const {
    return responseSerialNo;
}

T0201_0500_VersionMinus1And0& T0201_0500_VersionMinus1And0::setResponseSerialNo(int serialNo) {
    responseSerialNo = serialNo;
    return *this;
}

// T0201_0500_Version1 类方法实现
int T0201_0500_Version1::getWarnBit() const {
    return warnBit;
}

T0201_0500_Version1& T0201_0500_Version1::setWarnBit(int bit) {
    warnBit = bit;
    return *this;
}

int T0201_0500_Version1::getStatusBit() const {
    return statusBit;
}

T0201_0500_Version1& T0201_0500_Version1::setStatusBit(int bit) {
    statusBit = bit;
    return *this;
}

int T0201_0500_Version1::getLatitude() const {
    return latitude;
}

T0201_0500_Version1& T0201_0500_Version1::setLatitude(int lat) {
    latitude = lat;
    return *this;
}

int T0201_0500_Version1::getLongitude() const {
    return longitude;
}

T0201_0500_Version1& T0201_0500_Version1::setLongitude(int lon) {
    longitude = lon;
    return *this;
}

int T0201_0500_Version1::getAltitude() const {
    return altitude;
}

T0201_0500_Version1& T0201_0500_Version1::setAltitude(int alt) {
    altitude = alt;
    return *this;
}

int T0201_0500_Version1::getSpeed() const {
    return speed;
}

T0201_0500_Version1& T0201_0500_Version1::setSpeed(int spd) {
    speed = spd;
    return *this;
}

int T0201_0500_Version1::getDirection() const {
    return direction;
}

T0201_0500_Version1& T0201_0500_Version1::setDirection(int dir) {
    direction = dir;
    return *this;
}

LocalDateTime T0201_0500_Version1::getDeviceTime() const {
    return deviceTime;
}

T0201_0500_Version1& T0201_0500_Version1::setDeviceTime(LocalDateTime time) {
    deviceTime = time;
    return *this;
}

std::map<int, std::shared_ptr<void>> T0201_0500_Version1::getAttributes() const {
    return attributes;
}

T0201_0500_Version1& T0201_0500_Version1::setAttributes(const std::map<int, std::shared_ptr<void>>& attrs) {
    attributes = attrs;
    return *this;
}

int T0201_0500_Version1::getAttributeInt(int key) const {
    auto it = attributes.find(key);
    if (it != attributes.end() && it->second) {
        return *static_cast<int*>(it->second.get());
    }
    return 0;
}

long T0201_0500_Version1::getAttributeLong(int key) const {
    auto it = attributes.find(key);
    if (it != attributes.end() && it->second) {
        return *static_cast<long*>(it->second.get());
    }
    return 0L;
}

double T0201_0500_Version1::getLng() const {
    return longitude / 1000000.0;
}

double T0201_0500_Version1::getLat() const {
    return latitude / 1000000.0;
}

float T0201_0500_Version1::getSpeedKph() const {
    return speed / 10.0f;
}

int T0201_0500_Version1::getResponseSerialNo() const {
    return responseSerialNo;
}

T0201_0500_Version1& T0201_0500_Version1::setResponseSerialNo(int serialNo) {
    responseSerialNo = serialNo;
    return *this;
}
