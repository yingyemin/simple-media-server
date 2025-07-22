#ifndef T0704_H
#define T0704_H

#include <vector>
#include "T0200.h"

class T0704 {
private:
    int total;               // 数据项个数，占用 2 字节
    int type;                // 位置数据类型：0.正常位置批量汇报 1.盲区补报，占用 1 字节
    std::vector<std::shared_ptr<T0200_VersionMinus1And0>> items; // 位置汇报数据项，总单元 2 字节（每个元素指针大小通常为 4 或 8 字节，实际数据依赖 T0200）
    std::vector<std::shared_ptr<T0200_Version1>> items2019; // 位置汇报数据项，总单元 2 字节（每个元素指针大小通常为 4 或 8 字节，实际数据依赖 T0200）

public:
    T0704();
    ~T0704();

    // Getter 方法
    int getTotal() const;
    int getType() const;
    const std::vector<std::shared_ptr<T0200_VersionMinus1And0>>& getItems() const;
    const std::vector<std::shared_ptr<T0200_Version1>>& getItems2019() const;

    // Setter 方法，支持链式调用
    T0704& setTotal(int total);
    T0704& setType(int type);
    T0704& setItems(const std::vector<std::shared_ptr<T0200_VersionMinus1And0>>& items);
    T0704& setItems2019(const std::vector<std::shared_ptr<T0200_Version1>>& items);
    // 用于添加单个 T0200 元素的方法
    T0704& addItem(std::shared_ptr<T0200_VersionMinus1And0> item);
    T0704& addItem(std::shared_ptr<T0200_Version1> item);

};

#endif // T0704_H
