#ifndef T0302_H
#define T0302_H

class T0302 {
private:
    int responseSerialNo; // 应答流水号，占用 2 字节
    int answerId; // 答案 ID，占用 1 字节

public:
    T0302();
    ~T0302();

    // 获取应答流水号
    int getResponseSerialNo() const;
    // 设置应答流水号，支持链式调用
    T0302& setResponseSerialNo(int serialNo);

    // 获取答案 ID
    int getAnswerId() const;
    // 设置答案 ID，支持链式调用
    T0302& setAnswerId(int id);
};

#endif // T0302_H
