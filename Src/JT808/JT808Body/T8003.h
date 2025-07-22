#ifndef T8003_H
#define T8003_H

#include <vector>

// 版本 -1 和 0 的 T8003 类
class T8003_Version_1_0 {
private:
    int responseSerialNo; // 原始消息流水号，占用 2 字节
    std::vector<short> id; // 重传包 ID 列表，每个元素占用 1 字节

public:
    T8003_Version_1_0();
    ~T8003_Version_1_0();

    // Getter 方法
    int getResponseSerialNo() const;
    const std::vector<short>& getId() const;

    // Setter 方法，支持链式调用
    T8003_Version_1_0& setResponseSerialNo(int newResponseSerialNo);
    T8003_Version_1_0& setId(const std::vector<short>& newId);
    T8003_Version_1_0& addId(short newId);
};

// 版本 1 的 T8003 类
class T8003_Version_1 {
private:
    int responseSerialNo; // 原始消息流水号，占用 2 字节
    std::vector<short> id; // 重传包 ID 列表，每个元素占用 2 字节

public:
    T8003_Version_1();
    ~T8003_Version_1();

    // Getter 方法
    int getResponseSerialNo() const;
    const std::vector<short>& getId() const;

    // Setter 方法，支持链式调用
    T8003_Version_1& setResponseSerialNo(int newResponseSerialNo);
    T8003_Version_1& setId(const std::vector<short>& newId);
    T8003_Version_1& addId(short newId);
};

#endif // T8003_H
