#ifndef T8608_H
#define T8608_H

#include <vector>

class T8608 {
private:
    int type;         // 查询类型：1.圆形 2.矩形 3.多边形 4.路线，占用 1 字节
    std::vector<int> id; // 区域列表，总单元 4 字节

public:
    T8608();
    ~T8608() = default;

    // Getter 方法
    int getType() const;
    const std::vector<int>& getId() const;

    // Setter 方法，支持链式调用
    T8608& setType(int newType);
    T8608& setId(const std::vector<int>& newId);
    T8608& addId(int newId);
};

#endif // T8608_H
