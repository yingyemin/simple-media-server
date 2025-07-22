#ifndef T8300_H
#define T8300_H

#include <string>

class T8300 {
private:
    int sign;    // 标志，占用 1 字节
    int type;    // 类型，占用 1 字节，version 1 
    std::string content; // 文本信息，长度可变， version 0，1

public:
    T8300();
    ~T8300();

    // Getter 方法
    int getSign() const;
    int getType() const;
    const std::string& getContent() const;

    // Setter 方法，支持链式调用
    T8300& setSign(int newSign);
    T8300& setType(int newType);
    T8300& setContent(const std::string& newContent);
};

#endif // T8300_H
