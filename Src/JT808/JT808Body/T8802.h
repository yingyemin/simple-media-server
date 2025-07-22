#ifndef T8802_H
#define T8802_H

#include <string>

class T8802 {
private:
    int type;         // 多媒体类型：0.图像 1.音频 2.视频，占用 1 字节
    int channelId;    // 通道 ID(0 表示检索该媒体类型的所有通道)，占用 1 字节
    int event;        // 事件项编码：0.平台下发指令 1.定时动作 2.抢劫报警触发 3.碰撞侧翻报警触发 其他保留，占用 1 字节
    std::string startTime; // 起始时间(YYMMDDHHMMSS)，占用 6 字节（BCD 编码）
    std::string endTime;   // 结束时间(YYMMDDHHMMSS)，占用 6 字节（BCD 编码）

public:
    T8802();
    ~T8802() = default;

    // Getter 方法
    int getType() const;
    int getChannelId() const;
    int getEvent() const;
    std::string getStartTime() const;
    std::string getEndTime() const;

    // Setter 方法，支持链式调用
    T8802& setType(int newType);
    T8802& setChannelId(int newChannelId);
    T8802& setEvent(int newEvent);
    T8802& setStartTime(const std::string& newStartTime);
    T8802& setEndTime(const std::string& newEndTime);
};

#endif // T8802_H
