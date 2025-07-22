#ifndef T9202_H
#define T9202_H

#include <cstdint>
#include <string>

// 平台下发远程录像回放控制类
class T9202 {
public:
    // 构造函数
    T9202();

    // Getter 和 Setter 方法
    uint8_t getChannelNo() const;
    void setChannelNo(uint8_t no);

    uint8_t getPlaybackMode() const;
    void setPlaybackMode(uint8_t mode);

    uint8_t getPlaybackSpeed() const;
    void setPlaybackSpeed(uint8_t speed);

    std::string getPlaybackTime() const;
    void setPlaybackTime(const std::string& time);

private:
    // 逻辑通道号，占用 1 字节
    uint8_t channelNo;
    // 回放控制：0.开始回放 1.暂停回放 2.结束回放 3.快进回放 4.关键帧快退回放 5.拖动回放 6.关键帧播放，占用 1 字节
    uint8_t playbackMode;
    // 快进或快退倍数：0.无效 1.1 倍 2.2 倍 3.4 倍 4.8 倍 5.16 倍 (回放控制为 3 和 4 时,此字段内容有效,否则置 0)，占用 1 字节
    uint8_t playbackSpeed;
    // 拖动回放位置(YYMMDDHHMMSS,回放控制为 5 时,此字段有效)，在数据传输时以 6 字节 BCD 码表示
    std::string playbackTime;
};

#endif // T9202_H
