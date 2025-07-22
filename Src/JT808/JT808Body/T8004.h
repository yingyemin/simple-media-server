#ifndef T8004_H
#define T8004_H

#include <chrono>

class T8004 {
private:
    // 存储 UTC 时间，这里用 time_point 表示，对应 Java 的 LocalDateTime
    std::chrono::system_clock::time_point dateTime; // UTC 时间，对应 BCD 编码 6 字节

public:
    T8004();
    ~T8004();

    // Getter 方法
    std::chrono::system_clock::time_point getDateTime() const;

    // Setter 方法，支持链式调用
    T8004& setDateTime(const std::chrono::system_clock::time_point& newDateTime);
};

#endif // T8004_H
