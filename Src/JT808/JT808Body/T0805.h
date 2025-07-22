#ifndef T0805_H
#define T0805_H

#include <vector>

class T0805 {
private:
    int responseSerialNo; // 应答流水号，占用 2 字节
    int result;           // 结果：0.成功 1.失败 2.通道不支持，占用 1 字节
    std::vector<int> id;  // 多媒体 ID 列表，每个元素占用 4 字节，总单元 2 字节表示列表长度

public:
    T0805();
    ~T0805();

    // Getter 方法
    int getResponseSerialNo() const;
    int getResult() const;
    const std::vector<int>& getId() const;

    // Setter 方法，支持链式调用
    T0805& setResponseSerialNo(int newResponseSerialNo);
    T0805& setResult(int newResult);
    T0805& setId(const std::vector<int>& newId);
    T0805& addId(int newId);
};

#endif // T0805_H
