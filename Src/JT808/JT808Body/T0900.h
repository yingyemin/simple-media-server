#ifndef T0900_H
#define T0900_H

#include <memory>
#include <utility>
#include <map>
#include <string>
#include <vector>

class T0900 {
private:
    // 透传消息类型定义
    static constexpr int GNSSLocation = 0x00; // GNSS 模块详细定位数据
    static constexpr int ICCardInfo = 0x0B;   // 道路运输证 IC 卡信息
    static constexpr int SerialPortOne = 0x41;// 串口 1 透传消息
    static constexpr int SerialPortTwo = 0x42;// 串口 2 透传消息
    static constexpr int Custom = 0xF0;       // 用户自定义透传 0xF0~0xFF

    uint8_t type; //透传类型，占一个字节
    uint32_t totalLength; // 信息总长度，占4个字节，2011版本
    uint16_t msgLength; // 信息长度，占2个字节，2011版本
    std::vector<uint8_t> msgData; // 信息数据

public:
    T0900();
    ~T0900();

    // Getter 方法
    uint8_t getType() const;
    uint32_t getTotalLength() const;
    uint16_t getMsgLength() const;
    const std::vector<uint8_t>& getMsgData() const;

    // Setter 方法
    T0900& setType(uint8_t type);
    T0900& setTotalLength(uint32_t totalLength);
    T0900& setMsgLength(uint16_t msgLength);
    T0900& setMsgData(const std::vector<uint8_t>& msgData);

};

#endif // T0900_H
