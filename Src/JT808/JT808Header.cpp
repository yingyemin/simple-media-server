#include "JT808Header.h"

// Implement getters and setters for JT808Header
uint16_t JT808Header::getMsgId() const {
    return msgId;
}

void JT808Header::setMsgId(uint16_t id) {
    msgId = id;
}

uint16_t JT808Header::getProperties() const {
    return properties;
}

void JT808Header::setProperties(uint16_t prop) {
    properties = prop;
}

std::string JT808Header::getSimCode() {
    return simCode;
}

void JT808Header::setSimCode(const std::string& code) {
    simCode = code;
}

uint16_t JT808Header::getSerialNo() const {
    return serialNo;
}

void JT808Header::setSerialNo(uint16_t no) {
    serialNo = no;
}