#include "T1206.h"

// 构造函数，初始化成员变量
T1206::T1206() : responseSerialNo(0), result(0) {}

// 获取应答流水号
uint16_t T1206::getResponseSerialNo() const {
    return responseSerialNo;
}

// 设置应答流水号
void T1206::setResponseSerialNo(uint16_t no) {
    responseSerialNo = no;
}

// 获取结果
uint8_t T1206::getResult() const {
    return result;
}

// 设置结果
void T1206::setResult(uint8_t res) {
    result = res;
}

// 判断上传是否成功
bool T1206::isSuccess() const {
    return result == 0;
}
