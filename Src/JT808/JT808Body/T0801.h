#ifndef T0801_H
#define T0801_H

#include <memory>
// 假设 ByteBuf 对应的 C++ 类型为 std::vector<uint8_t>
#include <vector>
#include "T0200.h"
using ByteBuf = std::vector<uint8_t>;

class T0801 {
private:
    int id;           // 多媒体数据 ID，占用 4 字节
    int type;         // 多媒体类型：0.图像 1.音频 2.视频，占用 1 字节
    int format;       // 多媒体格式编码：0.JPEG 1.TIF 2.MP3 3.WAV 4.WMV，占用 1 字节
    int event;        // 事件项编码，占用 1 字节
    int channelId;    // 通道 ID，占用 1 字节
    std::unique_ptr<T0200_VersionMinus1And0> location; // 位置信息，占用 28 字节
    std::unique_ptr<T0200_Version1> location2019; // 位置信息，占用 28 字节
    std::unique_ptr<ByteBuf> packet;  // 多媒体数据包

public:
    T0801();
    ~T0801();

    // Getter 方法
    int getId() const;
    int getType() const;
    int getFormat() const;
    int getEvent() const;
    int getChannelId() const;
    const T0200_VersionMinus1And0* getLocation() const;
    const T0200_Version1* getLocation2019() const;
    const ByteBuf* getPacket() const;

    // Setter 方法，支持链式调用
    T0801& setId(int newId);
    T0801& setType(int newType);
    T0801& setFormat(int newFormat);
    T0801& setEvent(int newEvent);
    T0801& setChannelId(int newChannelId);
    T0801& setLocation(std::unique_ptr<T0200_VersionMinus1And0> newLocation);
    T0801& setLocation(std::unique_ptr<T0200_Version1> newLocation);
    T0801& setPacket(std::unique_ptr<ByteBuf> newPacket);

    bool noBuffer() const;
};

#endif // T0801_H
