#include "T0200.h"
#include <ctime>
#include <stdexcept>

// T0200_VersionMinus1And0 类成员函数实现

int T0200_VersionMinus1And0::getWarnBit() const {
    return warnBit;
}

void T0200_VersionMinus1And0::setWarnBit(int bit) {
    warnBit = bit;
}

int T0200_VersionMinus1And0::getStatusBit() const {
    return statusBit;
}

void T0200_VersionMinus1And0::setStatusBit(int bit) {
    statusBit = bit;
}

int T0200_VersionMinus1And0::getLatitude() const {
    return latitude;
}

void T0200_VersionMinus1And0::setLatitude(int lat) {
    latitude = lat;
}

int T0200_VersionMinus1And0::getLongitude() const {
    return longitude;
}

void T0200_VersionMinus1And0::setLongitude(int lon) {
    longitude = lon;
}

int T0200_VersionMinus1And0::getAltitude() const {
    return altitude;
}

void T0200_VersionMinus1And0::setAltitude(int alt) {
    altitude = alt;
}

int T0200_VersionMinus1And0::getSpeed() const {
    return speed;
}

void T0200_VersionMinus1And0::setSpeed(int spd) {
    speed = spd;
}

int T0200_VersionMinus1And0::getDirection() const {
    return direction;
}

void T0200_VersionMinus1And0::setDirection(int dir) {
    direction = dir;
}

std::string T0200_VersionMinus1And0::getDeviceTime() const {
    return deviceTime;
}

void T0200_VersionMinus1And0::setDeviceTime(std::string time) {
    deviceTime = time;
}

std::map<int, std::shared_ptr<AttributeConverter>> T0200_VersionMinus1And0::getAttributes() const {
    return attributes;
}

void T0200_VersionMinus1And0::setAttributes(const std::map<int, std::shared_ptr<AttributeConverter>>& attrs) {
    attributes = attrs;
}

std::shared_ptr<AttributeConverter> T0200_VersionMinus1And0::getAttributeInt(int key) const {
    auto it = attributes.find(key);
    if (it != attributes.end() && it->second) {
        return it->second;
    }
    return nullptr;
}

double T0200_VersionMinus1And0::getLng() const {
    return longitude / 1000000.0;
}

double T0200_VersionMinus1And0::getLat() const {
    return latitude / 1000000.0;
}

float T0200_VersionMinus1And0::getSpeedKph() const {
    return speed / 10.0f;
}

void T0200_VersionMinus1And0::parse(const uint8_t* data, size_t length) {
    if (length < 24) {
        throw std::invalid_argument("Data length is too short");
    }
    
    // 解析报警标志
    warnBit = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    // 解析状态
    statusBit = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
    // 解析纬度
    latitude = (data[8] << 24) | (data[9] << 16) | (data[10] << 8) | data[11];
    // 解析经度
    longitude = (data[12] << 24) | (data[13] << 16) | (data[14] << 8) | data[15];
    // 解析高程
    altitude = (data[16] << 8) | data[17];
    // 解析速度
    speed = (data[18] << 8) | data[19];
    // 解析方向
    direction = (data[20] << 8) | data[21];
    
    // 解析时间
    char timeStr[13];
    for (int i = 0; i < 6; ++i) {
        snprintf(timeStr + i * 2, 3, "%02X", data[22 + i]);
    }
    timeStr[12] = '\0';
    deviceTime = timeStr;
    
    // 解析位置附加信息
    size_t offset = 28;
    while (offset < length) {
        int key = data[offset++];
        int len = data[offset++];
        if (offset + len > length) {
            break;
        }
        auto converter = std::make_shared<AttributeConverter>();
        converter->id = key;
        converter->len = len;
        converter->data = new uint8_t[len];
        memcpy(converter->data, data + offset, len);
        attributes[key] = converter;
        offset += len;
    }
}

StringBuffer::Ptr T0200_VersionMinus1And0::encode() const {
    auto buffer = std::make_shared<StringBuffer>();
    
    // 编码报警标志
    buffer->push_back((warnBit >> 24) & 0xFF);
    buffer->push_back((warnBit >> 16) & 0xFF);
    buffer->push_back((warnBit >> 8) & 0xFF);
    buffer->push_back(warnBit & 0xFF);
    // 编码状态
    buffer->push_back((statusBit >> 24) & 0xFF);
    buffer->push_back((statusBit >> 16) & 0xFF);
    buffer->push_back((statusBit >> 8) & 0xFF);
    buffer->push_back(statusBit & 0xFF);
    // 编码纬度
    buffer->push_back((latitude >> 24) & 0xFF);
    buffer->push_back((latitude >> 16) & 0xFF);
    buffer->push_back((latitude >> 8) & 0xFF);
    buffer->push_back(latitude & 0xFF);
    // 编码经度
    buffer->push_back((longitude >> 24) & 0xFF);
    buffer->push_back((longitude >> 16) & 0xFF);
    buffer->push_back((longitude >> 8) & 0xFF);
    buffer->push_back(longitude & 0xFF);
    // 编码高程
    buffer->push_back((altitude >> 8) & 0xFF);
    buffer->push_back(altitude & 0xFF);
    // 编码速度
    buffer->push_back((speed >> 8) & 0xFF);
    buffer->push_back(speed & 0xFF);
    // 编码方向
    buffer->push_back((direction >> 8) & 0xFF);
    buffer->push_back(direction & 0xFF);
    
    // 编码时间
    for (size_t i = 0; i < deviceTime.length(); i += 2) {
        int val = strtol(deviceTime.substr(i, 2).c_str(), nullptr, 16);
        buffer->push_back(val);
    }
    
    // 编码位置附加信息
    for (const auto& pair : attributes) {
        buffer->push_back(pair.first);
        buffer->push_back(pair.second->len);
        buffer->append((char*)pair.second->data, pair.second->len);
    }
    
    return buffer;
}

// T0200_Version1 类成员函数实现

int T0200_Version1::getWarnBit() const {
    return warnBit;
}

void T0200_Version1::setWarnBit(int bit) {
    warnBit = bit;
}

int T0200_Version1::getStatusBit() const {
    return statusBit;
}

void T0200_Version1::setStatusBit(int bit) {
    statusBit = bit;
}

int T0200_Version1::getLatitude() const {
    return latitude;
}

void T0200_Version1::setLatitude(int lat) {
    latitude = lat;
}

int T0200_Version1::getLongitude() const {
    return longitude;
}

void T0200_Version1::setLongitude(int lon) {
    longitude = lon;
}

int T0200_Version1::getAltitude() const {
    return altitude;
}

void T0200_Version1::setAltitude(int alt) {
    altitude = alt;
}

int T0200_Version1::getSpeed() const {
    return speed;
}

void T0200_Version1::setSpeed(int spd) {
    speed = spd;
}

int T0200_Version1::getDirection() const {
    return direction;
}

void T0200_Version1::setDirection(int dir) {
    direction = dir;
}

std::string T0200_Version1::getDeviceTime() const {
    return deviceTime;
}

void T0200_Version1::setDeviceTime(std::string time) {
    deviceTime = time;
}

std::map<int, std::shared_ptr<AttributeConverterYue>> T0200_Version1::getAttributes() const {
    return attributes;
}

void T0200_Version1::setAttributes(const std::map<int, std::shared_ptr<AttributeConverterYue>>& attrs) {
    attributes = attrs;
}

std::shared_ptr<AttributeConverterYue> T0200_Version1::getAttributeInt(int key) const {
    auto it = attributes.find(key);
    if (it != attributes.end() && it->second) {
        return it->second;
    }
    return nullptr;
}

double T0200_Version1::getLng() const {
    return longitude / 1000000.0;
}

double T0200_Version1::getLat() const {
    return latitude / 1000000.0;
}

float T0200_Version1::getSpeedKph() const {
    return speed / 10.0f;
}

void T0200_Version1::parse(const uint8_t* data, size_t length) {
    if (length < 24) {
        throw std::invalid_argument("Data length is too short");
    }
    
    // 解析报警标志
    warnBit = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    // 解析状态
    statusBit = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
    // 解析纬度
    latitude = (data[8] << 24) | (data[9] << 16) | (data[10] << 8) | data[11];
    // 解析经度
    longitude = (data[12] << 24) | (data[13] << 16) | (data[14] << 8) | data[15];
    // 解析高程
    altitude = (data[16] << 8) | data[17];
    // 解析速度
    speed = (data[18] << 8) | data[19];
    // 解析方向
    direction = (data[20] << 8) | data[21];
    
    // 解析时间
    char timeStr[13];
    for (int i = 0; i < 6; ++i) {
        snprintf(timeStr + i * 2, 3, "%02X", data[22 + i]);
    }
    timeStr[12] = '\0';
    deviceTime = timeStr;
    
    // 解析位置附加信息(粤标)
    size_t offset = 28;
    while (offset < length) {
        int key = data[offset++];
        int len = data[offset++];
        if (offset + len > length) {
            break;
        }
        auto converter = std::make_shared<AttributeConverterYue>();
        converter->id = key;
        converter->len = len;
        converter->data = new uint8_t[len];
        memcpy(converter->data, data + offset, len);
        attributes[key] = converter;
        offset += len;
    }
}

StringBuffer::Ptr T0200_Version1::encode() const {
    auto buffer = std::make_shared<StringBuffer>();
    
    // 编码报警标志
    buffer->push_back((warnBit >> 24) & 0xFF);
    buffer->push_back((warnBit >> 16) & 0xFF);
    buffer->push_back((warnBit >> 8) & 0xFF);
    buffer->push_back(warnBit & 0xFF);
    // 编码状态
    buffer->push_back((statusBit >> 24) & 0xFF);
    buffer->push_back((statusBit >> 16) & 0xFF);
    buffer->push_back((statusBit >> 8) & 0xFF);
    buffer->push_back(statusBit & 0xFF);
    // 编码纬度
    buffer->push_back((latitude >> 24) & 0xFF);
    buffer->push_back((latitude >> 16) & 0xFF);
    buffer->push_back((latitude >> 8) & 0xFF);
    buffer->push_back(latitude & 0xFF);
    // 编码经度
    buffer->push_back((longitude >> 24) & 0xFF);
    buffer->push_back((longitude >> 16) & 0xFF);
    buffer->push_back((longitude >> 8) & 0xFF);
    buffer->push_back(longitude & 0xFF);
    // 编码高程
    buffer->push_back((altitude >> 8) & 0xFF);
    buffer->push_back(altitude & 0xFF);
    // 编码速度
    buffer->push_back((speed >> 8) & 0xFF);
    buffer->push_back(speed & 0xFF);
    // 编码方向
    buffer->push_back((direction >> 8) & 0xFF);
    buffer->push_back(direction & 0xFF);
    
    // 编码时间
    for (size_t i = 0; i < deviceTime.length(); i += 2) {
        int val = strtol(deviceTime.substr(i, 2).c_str(), nullptr, 16);
        buffer->push_back(val);
    }
    
    // 编码位置附加信息(粤标)
    for (const auto& pair : attributes) {
        buffer->push_back(pair.first);
        buffer->push_back(pair.second->len);
        buffer->append((char*)pair.second->data, pair.second->len);
    }
    
    return buffer;
}