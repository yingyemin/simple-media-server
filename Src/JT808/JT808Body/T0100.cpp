#include "T0100.h"
#include <sstream>

// T0100VNeg1 类方法实现
uint16_t T0100VNeg1::getProvinceId() const {
    return provinceId;
}

uint16_t T0100VNeg1::getCityId() const {
    return cityId;
}

const std::string& T0100VNeg1::getMakerId() const {
    return makerId;
}

const std::string& T0100VNeg1::getDeviceModel() const {
    return deviceModel;
}

const std::string& T0100VNeg1::getDeviceId() const {
    return deviceId;
}

uint8_t T0100VNeg1::getPlateColor() const {
    return plateColor;
}

const std::string& T0100VNeg1::getPlateNo() const {
    return plateNo;
}

void T0100VNeg1::setProvinceId(uint16_t id) {
    provinceId = id;
}

void T0100VNeg1::setCityId(uint16_t id) {
    cityId = id;
}

void T0100VNeg1::setMakerId(const std::string& id) {
    makerId = id.substr(0, 5); // 最多存储 5 字节
}

void T0100VNeg1::setDeviceModel(const std::string& model) {
    deviceModel = model.substr(0, 8); // 最多存储 8 字节
}

void T0100VNeg1::setDeviceId(const std::string& id) {
    deviceId = id.substr(0, 7); // 最多存储 7 字节
}

void T0100VNeg1::setPlateColor(uint8_t color) {
    plateColor = color;
}

void T0100VNeg1::setPlateNo(const std::string& no) {
    plateNo = no;
}

int T0100VNeg1::getProtocolVersion() const {
    return -1;
}

std::string T0100VNeg1::toString() const {
    std::ostringstream oss;
    oss << "T0100VNeg1["
        << "provinceId=" << provinceId << ", "
        << "cityId=" << cityId << ", "
        << "makerId=" << makerId << ", "
        << "deviceModel=" << deviceModel << ", "
        << "deviceId=" << deviceId << ", "
        << "plateColor=" << static_cast<int>(plateColor) << ", "
        << "plateNo=" << plateNo << "]";
    return oss.str();
}

void T0100VNeg1::parse(const char* buffer, size_t len) {
    size_t offset = 0;
    provinceId = static_cast<uint16_t>(buffer[offset] << 8 | buffer[offset + 1]);
    offset += 2;
    cityId = static_cast<uint16_t>(buffer[offset] << 8 | buffer[offset + 1]);
    offset += 2;

    makerId = std::string(reinterpret_cast<const char*>(buffer + offset), 5);
    offset += 5;

    deviceModel = std::string(reinterpret_cast<const char*>(buffer + offset), 8);
    offset += 8;

    deviceId = std::string(reinterpret_cast<const char*>(buffer + offset), 7);
    offset += 7;

    plateColor = buffer[offset];
    offset += 1;

    // 假设 plateNo 以 0 结尾
    size_t plateNoLen = len - offset - 1;
    plateNo = std::string(reinterpret_cast<const char*>(buffer + offset), plateNoLen);
    offset += plateNoLen + 1;
}

StringBuffer::Ptr T0100VNeg1::encode() const {
    StringBuffer::Ptr buffer = std::make_shared<StringBuffer>();
    buffer->push_back(static_cast<uint8_t>(provinceId >> 8));
    buffer->push_back(static_cast<uint8_t>(provinceId & 0xFF));

    buffer->push_back(static_cast<uint8_t>(cityId >> 8));
    buffer->push_back(static_cast<uint8_t>(cityId & 0xFF));

    buffer->append(makerId);

    buffer->append(deviceModel);

    buffer->append(deviceId);

    buffer->push_back(plateColor);

    buffer->append(plateNo);
    // buffer->push_back(0); // 以 0 结尾

    return buffer;
}

// T0100V0 类方法实现
uint16_t T0100V0::getProvinceId() const {
    return provinceId;
}

uint16_t T0100V0::getCityId() const {
    return cityId;
}

const std::string& T0100V0::getMakerId() const {
    return makerId;
}

const std::string& T0100V0::getDeviceModel() const {
    return deviceModel;
}

const std::string& T0100V0::getDeviceId() const {
    return deviceId;
}

uint8_t T0100V0::getPlateColor() const {
    return plateColor;
}

const std::string& T0100V0::getPlateNo() const {
    return plateNo;
}

void T0100V0::setProvinceId(uint16_t id) {
    provinceId = id;
}

void T0100V0::setCityId(uint16_t id) {
    cityId = id;
}

void T0100V0::setMakerId(const std::string& id) {
    makerId = id.substr(0, 5); // 最多存储 5 字节
}

void T0100V0::setDeviceModel(const std::string& model) {
    deviceModel = model.substr(0, 20); // 最多存储 20 字节
}

void T0100V0::setDeviceId(const std::string& id) {
    deviceId = id.substr(0, 7); // 最多存储 7 字节
}

void T0100V0::setPlateColor(uint8_t color) {
    plateColor = color;
}

void T0100V0::setPlateNo(const std::string& no) {
    plateNo = no;
}

int T0100V0::getProtocolVersion() const {
    return 0;
}

std::string T0100V0::toString() const {
    std::ostringstream oss;
    oss << "T0100V0["
        << "provinceId=" << provinceId << ", "
        << "cityId=" << cityId << ", "
        << "makerId=" << makerId << ", "
        << "deviceModel=" << deviceModel << ", "
        << "deviceId=" << deviceId << ", "
        << "plateColor=" << static_cast<int>(plateColor) << ", "
        << "plateNo=" << plateNo << "]";
    return oss.str();
}

void T0100V0::parse(const char* buffer, size_t len) {
    size_t offset = 0;
    provinceId = static_cast<uint16_t>(buffer[offset] << 8 | buffer[offset + 1]);
    offset += 2;
    cityId = static_cast<uint16_t>(buffer[offset] << 8 | buffer[offset + 1]);
    offset += 2;

    makerId = std::string(reinterpret_cast<const char*>(buffer + offset), 5);
    offset += 5;

    deviceModel = std::string(reinterpret_cast<const char*>(buffer + offset), 20);
    offset += 20;

    deviceId = std::string(reinterpret_cast<const char*>(buffer + offset), 7);
    offset += 7;

    plateColor = buffer[offset];
    offset += 1;

    // 假设 plateNo 以 0 结尾
    size_t plateNoLen = len - offset - 1;
    plateNo = std::string(reinterpret_cast<const char*>(buffer + offset), plateNoLen);
    offset += plateNoLen + 1;
}

StringBuffer::Ptr T0100V0::encode() const {
    StringBuffer::Ptr buffer = std::make_shared<StringBuffer>();
    buffer->push_back(static_cast<uint8_t>(provinceId >> 8));
    buffer->push_back(static_cast<uint8_t>(provinceId & 0xFF));

    buffer->push_back(static_cast<uint8_t>(cityId >> 8));
    buffer->push_back(static_cast<uint8_t>(cityId & 0xFF));

    buffer->append(makerId);

    buffer->append(deviceModel);

    buffer->append(deviceId);

    buffer->push_back(plateColor);

    buffer->append(plateNo);
    // buffer->push_back(0); // 以 0 结尾

    return buffer;
}

// T0100V1 类方法实现
uint16_t T0100V1::getProvinceId() const {
    return provinceId;
}

uint16_t T0100V1::getCityId() const {
    return cityId;
}

const std::string& T0100V1::getMakerId() const {
    return makerId;
}

const std::string& T0100V1::getDeviceModel() const {
    return deviceModel;
}

const std::string& T0100V1::getDeviceId() const {
    return deviceId;
}

uint8_t T0100V1::getPlateColor() const {
    return plateColor;
}

const std::string& T0100V1::getPlateNo() const {
    return plateNo;
}

void T0100V1::setProvinceId(uint16_t id) {
    provinceId = id;
}

void T0100V1::setCityId(uint16_t id) {
    cityId = id;
}

void T0100V1::setMakerId(const std::string& id) {
    makerId = id.substr(0, 11); // 最多存储 11 字节
}

void T0100V1::setDeviceModel(const std::string& model) {
    deviceModel = model.substr(0, 30); // 最多存储 30 字节
}

void T0100V1::setDeviceId(const std::string& id) {
    deviceId = id.substr(0, 30); // 最多存储 30 字节
}

void T0100V1::setPlateColor(uint8_t color) {
    plateColor = color;
}

void T0100V1::setPlateNo(const std::string& no) {
    plateNo = no;
}

int T0100V1::getProtocolVersion() const {
    return 1;
}

std::string T0100V1::toString() const {
    std::ostringstream oss;
    oss << "T0100V1["
        << "provinceId=" << provinceId << ", "
        << "cityId=" << cityId << ", "
        << "makerId=" << makerId << ", "
        << "deviceModel=" << deviceModel << ", "
        << "deviceId=" << deviceId << ", "
        << "plateColor=" << static_cast<int>(plateColor) << ", "
        << "plateNo=" << plateNo << "]";
    return oss.str();
}

void T0100V1::parse(const char* buffer, size_t len) {
    size_t offset = 0;
    provinceId = static_cast<uint16_t>(buffer[offset] << 8 | buffer[offset + 1]);
    offset += 2;
    cityId = static_cast<uint16_t>(buffer[offset] << 8 | buffer[offset + 1]);
    offset += 2;

    makerId = std::string(reinterpret_cast<const char*>(buffer + offset), 11);
    offset += 11;

    deviceModel = std::string(reinterpret_cast<const char*>(buffer + offset), 30);
    offset += 30;

    deviceId = std::string(reinterpret_cast<const char*>(buffer + offset), 30);
    offset += 30;

    plateColor = buffer[offset];
    offset += 1;

    // 假设 plateNo 以 0 结尾
    size_t plateNoLen = len - offset - 1;
    plateNo = std::string(reinterpret_cast<const char*>(buffer + offset), plateNoLen);
    offset += plateNoLen + 1;
}

StringBuffer::Ptr T0100V1::encode() const {
    StringBuffer::Ptr buffer = std::make_shared<StringBuffer>();
    buffer->push_back(static_cast<uint8_t>(provinceId >> 8));
    buffer->push_back(static_cast<uint8_t>(provinceId & 0xFF));

    buffer->push_back(static_cast<uint8_t>(cityId >> 8));
    buffer->push_back(static_cast<uint8_t>(cityId & 0xFF));

    buffer->append(makerId);

    buffer->append(deviceModel);

    buffer->append(deviceId);

    buffer->push_back(plateColor);

    buffer->append(plateNo);
    // buffer->push_back(0); // 以 0 结尾

    return buffer;
}
