#include "T0102.h"
#include <sstream>

// T0102VNeg1And0 类方法实现
const std::string& T0102VNeg1And0::getToken() const {
    return token;
}

void T0102VNeg1And0::setToken(const std::string& token) {
    this->token = token;
}

int T0102VNeg1And0::getProtocolVersion() const {
    return -1; // 这里表示 -1 和 0 版本，实际可根据需求调整逻辑
}

std::string T0102VNeg1And0::toString() const {
    std::ostringstream oss;
    oss << "T0102VNeg1And0["
        << "token=" << token << "]";
    return oss.str();
}

void T0102VNeg1And0::parse(const std::vector<uint8_t>& buffer, size_t& offset) {
    // 假设 token 以 0 结尾
    size_t tokenLen = buffer.size() - offset;
    token = std::string(reinterpret_cast<const char*>(buffer.data() + offset), tokenLen);
    offset += tokenLen + 1;
}

void T0102VNeg1And0::encode(std::vector<uint8_t>& buffer) const {
    buffer.insert(buffer.end(), token.begin(), token.end());
    buffer.push_back(0); // 以 0 结尾
}

// T0102V1 类方法实现
uint8_t T0102V1::getTokenLength() const {
    return tokenLength;
}

const std::string& T0102V1::getToken() const {
    return token;
}

const std::string& T0102V1::getImei() const {
    return imei;
}

const std::string& T0102V1::getSoftwareVersion() const {
    return softwareVersion;
}

void T0102V1::setTokenLength(uint8_t length) {
    tokenLength = length;
}

void T0102V1::setToken(const std::string& token) {
    this->token = token;
    tokenLength = static_cast<uint8_t>(token.size());
}

void T0102V1::setImei(const std::string& imei) {
    this->imei = imei.substr(0, 15);
}

void T0102V1::setSoftwareVersion(const std::string& version) {
    this->softwareVersion = version.substr(0, 20);
}

int T0102V1::getProtocolVersion() const {
    return 1;
}

std::string T0102V1::toString() const {
    std::ostringstream oss;
    oss << "T0102V1["
        << "tokenLength=" << static_cast<int>(tokenLength) << ", "
        << "token=" << token << ", "
        << "imei=" << imei << ", "
        << "softwareVersion=" << softwareVersion << "]";
    return oss.str();
}

void T0102V1::parse(const std::vector<uint8_t>& buffer, size_t& offset) {
    tokenLength = buffer[offset++];
    token = std::string(reinterpret_cast<const char*>(buffer.data() + offset), tokenLength);
    offset += tokenLength;

    imei = std::string(reinterpret_cast<const char*>(buffer.data() + offset), 15);
    offset += 15;

    softwareVersion = std::string(reinterpret_cast<const char*>(buffer.data() + offset), 20);
    offset += 20;
}

void T0102V1::encode(std::vector<uint8_t>& buffer) const {
    buffer.push_back(tokenLength);
    buffer.insert(buffer.end(), token.begin(), token.end());

    std::string paddedImei = imei.substr(0, 15);
    paddedImei.resize(15, '\0');
    buffer.insert(buffer.end(), paddedImei.begin(), paddedImei.end());

    std::string paddedSoftwareVersion = softwareVersion.substr(0, 20);
    paddedSoftwareVersion.resize(20, '\0');
    buffer.insert(buffer.end(), paddedSoftwareVersion.begin(), paddedSoftwareVersion.end());
}
