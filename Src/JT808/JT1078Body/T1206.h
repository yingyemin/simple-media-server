#ifndef T1206_H
#define T1206_H

#include <cstdint>

// 模拟 Response 类，这里简单用空类表示
class Response {};

// 模拟 JTMessage 类，这里简单用空类表示
class JTMessage {};

// 文件上传完成通知类
class T1206 : public JTMessage, public Response {
public:
    // 构造函数
    T1206();

    // Getter 和 Setter 方法
    uint16_t getResponseSerialNo() const;
    void setResponseSerialNo(uint16_t no);

    uint8_t getResult() const;
    void setResult(uint8_t result);

    // 判断上传是否成功
    bool isSuccess() const;

private:
    // 应答流水号，占用 2 字节
    uint16_t responseSerialNo;
    // 结果：0.成功 1.失败，占用 1 字节
    uint8_t result;
};

#endif // T1206_H
