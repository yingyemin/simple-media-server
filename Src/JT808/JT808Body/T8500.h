#ifndef T8500_H
#define T8500_H

class T8500 {
private:
    int type; // 控制标志和控制类型，版本 0 时占用 1 字节，版本 1 时控制类型占用 2 字节
    int param; // 控制标志，版本 1 时占用 1 字节

public:
    T8500();
    ~T8500();

    // Getter 方法
    int getType() const;
    int getParam() const;

    // Setter 方法，支持链式调用
    T8500& setType(int newType);
    T8500& setParam(int newParam);
};

#endif // T8500_H
