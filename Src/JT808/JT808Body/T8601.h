#ifndef T8601_H
#define T8601_H

#include <vector>

class T8601 {
private:
    std::vector<int> id; // 区域列表，总单元 1 字节表示列表长度

public:
    T8601();
    ~T8601();

    // Getter 方法
    const std::vector<int>& getId() const;

    // Setter 方法，支持链式调用
    T8601& setId(const std::vector<int>& newId);

    // 添加区域 ID 方法
    T8601& addId(int newId);
};

#endif // T8601_H
