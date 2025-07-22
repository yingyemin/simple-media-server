#ifndef T8900_H
#define T8900_H

#include <memory>
#include <string>
#include <utility>

class KeyValuePair {
public:
    int key;
    std::shared_ptr<void> value;

    KeyValuePair() : key(0), value(nullptr) {}
    KeyValuePair(int k, std::shared_ptr<void> v) : key(k), value(std::move(v)) {}
};

class PeripheralStatus {
public:
    static const int key = 0; // 假设值，根据实际情况修改
    // 这里可添加 PeripheralStatus 类的成员变量和方法
};

class PeripheralSystem {
public:
    static const int key = 1; // 假设值，根据实际情况修改
    // 这里可添加 PeripheralSystem 类的成员变量和方法
};

class JTMessage {
    // 这里可添加 JTMessage 类的成员变量和方法
};

class T8900 : public JTMessage {
public:
    // GNSS模块详细定位数据
    static const int GNSSLocation = 0x00;
    // 道路运输证IC卡信息上传消息为64Byte,下传消息为24Byte,道路运输证IC卡认证透传超时时间为30s.超时后,不重发
    static const int ICCardInfo = 0x0B;
    // 串口1透传消息
    static const int SerialPortOne = 0x41;
    // 串口2透传消息
    static const int SerialPortTwo = 0x42;
    // 用户自定义透传 0xF0~0xFF
    static const int Custom = 0xF0;

    KeyValuePair message; // 透传消息
    std::shared_ptr<PeripheralStatus> peripheralStatus; // 状态查询(外设状态信息：外设工作状态、设备报警信息)
    std::shared_ptr<PeripheralSystem> peripheralSystem; // 信息查询(外设传感器的基本信息：公司信息、产品代码、版本号、外设ID、客户代码)

    T8900();
    ~T8900() = default;

    T8900& build();
};

#endif // T8900_H
