#ifndef T0303_H
#define T0303_H

// 前向声明 JTMessage
class JTMessage;

class T0303 {
private:
    int type; // 消息类型，占用 1 字节
    int action; // 点播/取消标志：0.取消 1.点播，占用 1 字节

public:
    T0303();
    ~T0303();

    // 获取消息类型
    int getType() const;
    // 设置消息类型，支持链式调用
    T0303& setType(int t);

    // 获取点播/取消标志
    int getAction() const;
    // 设置点播/取消标志，支持链式调用
    T0303& setAction(int a);
};

#endif // T0303_H
