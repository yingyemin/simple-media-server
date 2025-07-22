#ifndef T8108_H
#define T8108_H

#include <string>
#include <vector>

class T8108 {
private:
    // 升级类型常量
    static constexpr int Terminal = 0;
    static constexpr int CardReader = 12;
    static constexpr int Beidou = 52;

    int type;           // 升级类型，占用 1 字节
    std::string makerId;// 制造商 ID, 终端制造商编码，占用 5 字节
    std::string version;// 版本号，长度可变
    std::vector<uint8_t> packet; // 数据包，长度可变

public:
    T8108();
    ~T8108();

    // 获取静态常量值
    static int getTerminal();
    static int getCardReader();
    static int getBeidou();

    // Getter 方法
    int getType() const;
    const std::string& getMakerId() const;
    const std::string& getVersion() const;
    const std::vector<uint8_t>& getPacket() const;

    // Setter 方法，支持链式调用
    T8108& setType(int newType);
    T8108& setMakerId(const std::string& newMakerId);
    T8108& setVersion(const std::string& newVersion);
    T8108& setPacket(const std::vector<uint8_t>& newPacket);
};

#endif // T8108_H
