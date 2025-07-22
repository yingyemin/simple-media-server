#ifndef T9205_H
#define T9205_H

#include <cstdint>
#include <string>

// 平台下发查询资源列表类
class T9205 {
public:
    // 构造函数，初始化成员变量
    T9205();

    // Getter 方法
    uint8_t getChannelNo() const;
    std::string getStartTime() const;
    std::string getEndTime() const;
    uint32_t getWarnBit1() const;
    uint32_t getWarnBit2() const;
    uint8_t getMediaType() const;
    uint8_t getStreamType() const;
    uint8_t getStorageType() const;

    // Setter 方法
    void setChannelNo(uint8_t no);
    void setStartTime(const std::string& time);
    void setEndTime(const std::string& time);
    void setWarnBit1(uint32_t bit);
    void setWarnBit2(uint32_t bit);
    void setMediaType(uint8_t type);
    void setStreamType(uint8_t type);
    void setStorageType(uint8_t type);

private:
    // 逻辑通道号，占用 1 字节
    uint8_t channelNo;
    // 开始时间，格式为 YYMMDDHHMMSS，全 0 表示无起始时间，数据传输时为 6 字节 BCD 码
    std::string startTime;
    // 结束时间，格式为 YYMMDDHHMMSS，全 0 表示无终止时间，数据传输时为 6 字节 BCD 码
    std::string endTime;
    // 报警标志 0 - 31 位，参考 808 协议文档报警标志位定义，占用 4 字节
    uint32_t warnBit1;
    // 报警标志 32 - 63 位，占用 4 字节
    uint32_t warnBit2;
    // 音视频资源类型，0. 音视频 1. 音频 2. 视频 3. 视频或音视频，占用 1 字节
    uint8_t mediaType;
    // 码流类型，0. 所有码流 1. 主码流 2. 子码流，占用 1 字节
    uint8_t streamType;
    // 存储器类型，0. 所有存储器 1. 主存储器 2. 灾备存储器，占用 1 字节
    uint8_t storageType;
};

#endif // T9205_H
