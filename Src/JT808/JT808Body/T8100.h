#ifndef T8100_H
#define T8100_H

#include <string>
#include "Net/Buffer.h"

// 注册应答
class T8100 {
private:
    // 应答结果常量
    static constexpr int Success = 0;
    static constexpr int AlreadyRegisteredVehicle = 1;
    static constexpr int NotFoundVehicle = 2;
    static constexpr int AlreadyRegisteredTerminal = 3;
    static constexpr int NotFoundTerminal = 4;

    int responseSerialNo; // 应答流水号，占用 2 字节
    int resultCode;       // 结果，占用 1 字节
    std::string token;    // 鉴权码，成功后才有该字段

public:
    T8100();
    ~T8100();

    // 获取静态常量值
    static int getSuccess();
    static int getAlreadyRegisteredVehicle();
    static int getNotFoundVehicle();
    static int getAlreadyRegisteredTerminal();
    static int getNotFoundTerminal();

    // Getter 方法
    int getResponseSerialNo() const;
    int getResultCode() const;
    const std::string& getToken() const;

    // Setter 方法，支持链式调用
    T8100& setResponseSerialNo(int newResponseSerialNo);
    T8100& setResultCode(int newResultCode);
    T8100& setToken(const std::string& newToken);

    void parse(const char* data, int len);
    StringBuffer::Ptr encode();
};

#endif // T8100_H