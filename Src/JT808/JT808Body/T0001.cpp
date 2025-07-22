#include "T0001.h"

// Getter 方法实现
uint16_t T0001::getResponseSerialNo() const {
    return responseSerialNo;
}

uint16_t T0001::getResponseMessageId() const {
    return responseMessageId;
}

uint8_t T0001::getResultCode() const {
    return resultCode;
}

// Setter 方法实现
void T0001::setResponseSerialNo(uint16_t id) {
    responseSerialNo = id;
}

void T0001::setResponseMessageId(uint16_t id) {
    responseMessageId = id;
}

void T0001::setResultCode(uint8_t code) {
    resultCode = code;
}

bool T0001::isSuccess() const {
    return resultCode == Success;
}

// 从字节流解析数据填充对象
void T0001::parse(const char* buffer, size_t len) {
    if (len < 5) {
        return;
    }

    int offset = 0;
    responseSerialNo = static_cast<uint16_t>(buffer[offset] << 8 | buffer[offset + 1]);
    offset += 2;

    responseMessageId = static_cast<uint16_t>(buffer[offset] << 8 | buffer[offset + 1]);
    offset += 2;

    resultCode = buffer[offset];
    offset += 1;
}

// 将对象数据编码为字节流
StringBuffer::Ptr T0001::encode() {
    auto buffer = std::make_shared<StringBuffer>();

    buffer->push_back(static_cast<uint8_t>(responseSerialNo >> 8));
    buffer->push_back(static_cast<uint8_t>(responseSerialNo & 0xFF));

    buffer->push_back(static_cast<uint8_t>(responseMessageId >> 8));
    buffer->push_back(static_cast<uint8_t>(responseMessageId & 0xFF));

    buffer->push_back(resultCode);

    return buffer;
}
