#include "T8100.h"

// 默认构造函数
T8100::T8100() : responseSerialNo(0), resultCode(0) {}

// 析构函数
T8100::~T8100() = default;

// 获取静态常量值的方法
int T8100::getSuccess() {
    return Success;
}

int T8100::getAlreadyRegisteredVehicle() {
    return AlreadyRegisteredVehicle;
}

int T8100::getNotFoundVehicle() {
    return NotFoundVehicle;
}

int T8100::getAlreadyRegisteredTerminal() {
    return AlreadyRegisteredTerminal;
}

int T8100::getNotFoundTerminal() {
    return NotFoundTerminal;
}

// Getter 方法实现
int T8100::getResponseSerialNo() const {
    return responseSerialNo;
}

int T8100::getResultCode() const {
    return resultCode;
}

const std::string& T8100::getToken() const {
    return token;
}

// Setter 方法实现，支持链式调用
T8100& T8100::setResponseSerialNo(int newResponseSerialNo) {
    responseSerialNo = newResponseSerialNo;
    return *this;
}

T8100& T8100::setResultCode(int newResultCode) {
    resultCode = newResultCode;
    return *this;
}

T8100& T8100::setToken(const std::string& newToken) {
    token = newToken;
    return *this;
}

// 解析传入的数据
void T8100::parse(const char* data, int len) {
    if (len < 3) return; // 至少需要3字节（2字节应答流水号 + 1字节结果码）
    
    // 解析应答流水号（2字节）
    responseSerialNo = (static_cast<unsigned char>(data[0]) << 8) | static_cast<unsigned char>(data[1]);
    
    // 解析结果码（1字节）
    resultCode = static_cast<unsigned char>(data[2]);
    
    // 如果结果码为成功，解析鉴权码
    if (resultCode == Success && len > 3) {
        token = std::string(data + 3, len - 3);
    }
}

// 编码对象数据为字节流
StringBuffer::Ptr T8100::encode() {
    auto buffer = std::make_shared<StringBuffer>();
    
    // 写入应答流水号（2字节）
    buffer->push_back(static_cast<uint8_t>(responseSerialNo >> 8));
    buffer->push_back(static_cast<uint8_t>(responseSerialNo & 0xFF));
    
    // 写入结果码（1字节）
    buffer->push_back(static_cast<uint8_t>(resultCode));
    
    // 如果结果码为成功，写入鉴权码
    if (resultCode == Success) {
        buffer->append(token);
    }
    
    return buffer;
}