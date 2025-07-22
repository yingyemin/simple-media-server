#ifndef T8106_H
#define T8106_H

#include <vector>

class T8106 {
private:
    std::vector<int> id; // 参数 ID 列表，每个元素占用 4 字节，总单元 1 字节表示列表长度

public:
    T8106();
    ~T8106();

    // Getter 方法
    const std::vector<int>& getId() const;

    // Setter 方法，支持链式调用
    T8106& setId(const std::vector<int>& newId);
    T8106& addId(int newId);
};

#endif // T8106_H
