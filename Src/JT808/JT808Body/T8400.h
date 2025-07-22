#ifndef T8400_H
#define T8400_H

#include <string>

class T8400 {
private:
    // 通话类型常量
    static constexpr int Normal = 0;
    static constexpr int Listen = 1;

    int type; // 类型：0.通话 1.监听，占用 1 字节
    std::string phoneNumber; // 电话号码，占用 20 字节

public:
    T8400();
    ~T8400();

    // 获取静态常量值
    static int getNormal();
    static int getListen();

    // Getter 方法
    int getType() const;
    const std::string& getPhoneNumber() const;

    // Setter 方法，支持链式调用
    T8400& setType(int newType);
    T8400& setPhoneNumber(const std::string& newPhoneNumber);
};

#endif // T8400_H
