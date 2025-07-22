#ifndef T8805_H
#define T8805_H

class T8805 {
private:
    int id;       // 多媒体 ID(大于 0)，占用 4 字节
    int deleteFlag; // 删除标志：0.保留 1.删除 ，占用 1 字节

public:
    T8805();
    ~T8805() = default;

    // Getter 方法
    int getId() const;
    int getDeleteFlag() const;

    // Setter 方法，支持链式调用
    T8805& setId(int newId);
    T8805& setDeleteFlag(int newDeleteFlag);
};

#endif // T8805_H
