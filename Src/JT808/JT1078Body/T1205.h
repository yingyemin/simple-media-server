#ifndef T1205_H
#define T1205_H

#include <cstdint>
#include <vector>
#include <chrono>

// 音视频资源列表项类
class Item {
public:
    // 构造函数
    Item();

    // Getter 和 Setter 方法
    uint8_t getChannelNo() const;
    void setChannelNo(uint8_t no);

    std::chrono::system_clock::time_point getStartTime() const;
    void setStartTime(const std::chrono::system_clock::time_point& time);

    std::chrono::system_clock::time_point getEndTime() const;
    void setEndTime(const std::chrono::system_clock::time_point& time);

    uint64_t getWarnBit() const;
    void setWarnBit(uint64_t bit);

    uint8_t getMediaType() const;
    void setMediaType(uint8_t type);

    uint8_t getStreamType() const;
    void setStreamType(uint8_t type);

    uint8_t getStorageType() const;
    void setStorageType(uint8_t type);

    uint32_t getSize() const;
    void setSize(uint32_t size);

private:
    // 逻辑通道号，占用 1 字节
    uint8_t channelNo;
    // 开始时间，在数据传输时以 6 字节 BCD 码表示
    std::chrono::system_clock::time_point startTime;
    // 结束时间，在数据传输时以 6 字节 BCD 码表示
    std::chrono::system_clock::time_point endTime;
    // 报警标志 0~63，占用 8 字节
    uint64_t warnBit;
    // 音视频资源类型，占用 1 字节
    uint8_t mediaType;
    // 码流类型，占用 1 字节
    uint8_t streamType;
    // 存储器类型，占用 1 字节
    uint8_t storageType;
    // 文件大小，占用 4 字节
    uint32_t size;
};

// 终端上传音视频资源列表类
class T1205 {
public:
    // 构造函数
    T1205();

    // Getter 和 Setter 方法
    uint16_t getResponseSerialNo() const;
    void setResponseSerialNo(uint16_t no);

    std::vector<Item> getItems() const;
    void setItems(const std::vector<Item>& items);

private:
    // 应答流水号，占用 2 字节
    uint16_t responseSerialNo;
    // 音视频资源列表，占用 4 字节表示总长度
    std::vector<Item> items;
};

#endif // T1205_H
