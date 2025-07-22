#ifndef T0608_H
#define T0608_H

// 前向声明 JTMessage
class JTMessage;

class T0608 {
private:
    int type; // 查询类型，占用 1 字节
    unsigned char* bytes; // 查询返回的数据，总单元 4
    int bytesLength; // 记录 bytes 数组的长度

public:
    T0608();
    ~T0608();

    // 获取查询类型
    int getType() const;
    // 设置查询类型，支持链式调用
    T0608& setType(int t);

    // 获取查询返回的数据
    const unsigned char* getBytes() const;
    // 设置查询返回的数据，支持链式调用
    T0608& setBytes(const unsigned char* data, int length);
    // 获取数据长度
    int getBytesLength() const;
};

#endif // T0608_H
