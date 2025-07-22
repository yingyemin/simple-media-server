#ifndef T0700_H
#define T0700_H

#include <vector>

class T0700 {
private:
    int responseSerialNo; // 应答流水号，占用 2 字节
    int command; // 命令字，占用 1 字节
    std::vector<unsigned char> data; // 数据块

public:
    T0700();
    ~T0700();

    // 获取应答流水号
    int getResponseSerialNo() const;
    // 设置应答流水号，支持链式调用
    T0700& setResponseSerialNo(int serialNo);

    // 获取命令字
    int getCommand() const;
    // 设置命令字，支持链式调用
    T0700& setCommand(int cmd);

    // 获取数据块
    const std::vector<unsigned char>& getData() const;
    // 设置数据块，支持链式调用
    T0700& setData(const std::vector<unsigned char>& newData);
};

#endif // T0700_H
