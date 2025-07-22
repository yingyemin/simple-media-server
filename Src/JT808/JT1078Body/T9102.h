#ifndef T9102_H
#define T9102_H

#include <cstdint>
#include "Net/Buffer.h"

// 音视频实时传输控制类
class T9102 {
public:
    // 构造函数
    T9102();

    // Getter 和 Setter 方法
    uint8_t getChannelNo() const;
    void setChannelNo(uint8_t no);

    uint8_t getCommand() const;
    void setCommand(uint8_t cmd);

    uint8_t getCloseType() const;
    void setCloseType(uint8_t type);

    uint8_t getStreamType() const;
    void setStreamType(uint8_t type);

    void parse(const char* buf, size_t size);
    StringBuffer::Ptr encode();

private:
    // 逻辑通道号，占用 1 字节
    uint8_t channelNo;
    // 控制指令，占用 1 字节
    // " 0.关闭音视频传输指令" +
    //         " 1.切换码流(增加暂停和继续)" +
    //         " 2.暂停该通道所有流的发送" +
    //         " 3.恢复暂停前流的发送,与暂停前的流类型一致" +
    //         " 4.关闭双向对讲")
    uint8_t command;
    //  " 0.关闭该通道有关的音视频数据" +
    //         " 1.只关闭该通道有关的音频,保留该通道有关的视频" +
    //         " 2.只关闭该通道有关的视频,保留该通道有关的音频")
    // 关闭音视频类型，占用 1 字节
    uint8_t closeType;
    // 切换码流类型，占用 1 字节
    uint8_t streamType;
};

#endif // T9102_H
