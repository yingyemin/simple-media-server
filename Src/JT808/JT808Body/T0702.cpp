#include "T0702.h"

// 默认构造函数
T0702::T0702() : status(0), cardStatus(0) {}

// 析构函数
T0702::~T0702() = default;

// Getter 方法实现
int T0702::getStatus() const {
    return status;
}

std::string T0702::getDateTime() const {
    return dateTime;
}

int T0702::getCardStatus() const {
    return cardStatus;
}

std::string T0702::getName() const {
    return name;
}

std::string T0702::getLicenseNo() const {
    return licenseNo;
}

std::string T0702::getInstitution() const {
    return institution;
}

std::string T0702::getLicenseValidPeriod() const {
    return licenseValidPeriod;
}

std::string T0702::getIdCard() const {
    return idCard;
}

// Setter 方法实现，支持链式调用
T0702& T0702::setStatus(int newStatus) {
    status = newStatus;
    return *this;
}

T0702& T0702::setDateTime(const std::string& newDateTime) {
    dateTime = newDateTime;
    return *this;
}

T0702& T0702::setCardStatus(int newCardStatus) {
    cardStatus = newCardStatus;
    return *this;
}

T0702& T0702::setName(const std::string& newName) {
    name = newName;
    return *this;
}

T0702& T0702::setLicenseNo(const std::string& newLicenseNo) {
    licenseNo = newLicenseNo;
    return *this;
}

T0702& T0702::setInstitution(const std::string& newInstitution) {
    institution = newInstitution;
    return *this;
}

T0702& T0702::setLicenseValidPeriod(const std::string& newLicenseValidPeriod) {
    licenseValidPeriod = newLicenseValidPeriod;
    return *this;
}

T0702& T0702::setIdCard(const std::string& newIdCard) {
    idCard = newIdCard;
    return *this;
}
