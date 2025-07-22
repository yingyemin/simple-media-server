#ifndef T0301_H
#define T0301_H

class T0301 {
private:
    // 事件 ID，占用 1 字节
    int eventId;

public:
    T0301();
    ~T0301();

    // 获取事件 ID
    int getEventId() const;
    // 设置事件 ID，支持链式调用
    T0301& setEventId(int id);
};

#endif // T0301_H
