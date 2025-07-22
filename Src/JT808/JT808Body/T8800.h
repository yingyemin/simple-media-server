#ifndef T8800_H
#define T8800_H

#include <vector>

class T8800 {
private:
    int mediaId; // 多媒体 ID(大于 0) 如收到全部数据包则没有后续字段，占用 4 字节
    std::vector<short> id; // 重传包 ID 列表，总单元 1 字节

public:
    T8800();
    ~T8800() = default;

    // Getter 方法
    int getMediaId() const;
    const std::vector<short>& getId() const;

    // Setter 方法，支持链式调用
    T8800& setMediaId(int newMediaId);
    T8800& setId(const std::vector<short>& newId);
    T8800& addId(short newId);
};

#endif // T8800_H
