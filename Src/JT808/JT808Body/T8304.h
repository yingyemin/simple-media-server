#ifndef T8304_H
#define T8304_H

#include <string>

class T8304 {
private:
    int type;        // 信息类型，占用 1 字节
    std::string content; // 文本信息，长度可变，以 2 字节为单位

public:
    T8304();
    ~T8304();

    // Getter 方法
    int getType() const;
    const std::string& getContent() const;

    // Setter 方法，支持链式调用
    T8304& setType(int newType);
    T8304& setContent(const std::string& newContent);
};

#endif // T8304_H
