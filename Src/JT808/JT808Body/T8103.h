#ifndef T8103_H
#define T8103_H

#include <map>
#include <string>
#include <memory>
#include <functional>

class ParameterConverter {
public:
    uint32_t paramId; // 参数ID，4字节
    uint8_t paramLen; // 参数长度，1字节
    // 参数值有int类型和string类型，所以分成两个参数
    uint32_t paramValueInt; // 参数值，paramLen字节
    std::string paramValueString; // 参数数据，paramLen字节
};

// 设置终端参数
class T8103 {
private:
    uint8_t num; // 参数总数，占一个字节
    std::map<uint8_t, std::shared_ptr<ParameterConverter>> parameters; // 参数项列表，每个键值对根据实际类型确定字节数

public:
    T8103();
    ~T8103();
};

#endif // T8103_H
