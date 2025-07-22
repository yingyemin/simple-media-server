#ifndef T0901_H
#define T0901_H

#include <vector>
#include <cstdint>

class T0901 {
private:
    uint32_t bodyLength;
    std::vector<unsigned char> body; // 压缩消息体，每个元素占用 1 字节，总长度可变

public:
    T0901();
    ~T0901();

    // Getter 方法
    const std::vector<unsigned char>& getBody() const;

    // Setter 方法，支持链式调用
    T0901& setBody(const std::vector<unsigned char>& newBody);
};

#endif // T0901_H
