#ifndef T9105_H
#define T9105_H

#include <cstdint>

// 实时音视频传输状态通知类
class T9105 {
public:
    // 构造函数
    T9105();

    // Getter 方法
    uint8_t getChannelNo() const;
    uint8_t getPacketLossRate() const;

    // Setter 方法
    void setChannelNo(uint8_t no);
    void setPacketLossRate(uint8_t rate);

private:
    // 逻辑通道号，占用 1 字节
    uint8_t channelNo;
    // 丢包率，占用 1 字节
    uint8_t packetLossRate;
};

#endif // T9105_H
