#ifndef T8803_H
#define T8803_H

#include <string>

class T8803 {
private:
    int type;          // 多媒体类型：0.图像 1.音频 2.视频，占用 1 字节
    int channelId;     // 通道 ID，占用 1 字节
    int event;         // 事件项编码：0.平台下发指令 1.定时动作 2.抢劫报警触发 3.碰撞侧翻报警触发 其他保留，占用 1 字节
    std::string startTime; // 起始时间(YYMMDDHHMMSS)，占用 6 字节（BCD 编码）
    std::string endTime;   // 结束时间(YYMMDDHHMMSS)，占用 6 字节（BCD 编码）
    int deleteFlag;    // 删除标志：0.保留 1.删除，占用 1 字节

public:
    T8803();
    ~T8803() = default;

    // Getter 方法
    int getType() const;
    int getChannelId() const;
    int getEvent() const;
    std::string getStartTime() const;
    std::string getEndTime() const;
    int getDeleteFlag() const;

    // Setter 方法，支持链式调用
    T8803& setType(int newType);
    T8803& setChannelId(int newChannelId);
    T8803& setEvent(int newEvent);
    T8803& setStartTime(const std::string& newStartTime);
    T8803& setEndTime(const std::string& newEndTime);
    T8803& setDeleteFlag(int newDeleteFlag);
};

#endif // T8803_H
