#ifndef T0A00_8A00_H
#define T0A00_8A00_H

#include <cstdint>
#include <string>
#include <vector>

/**
 * 对应 Java 中的 T0A00_8A00 类，代表平台 RSA 公钥和终端 RSA 公钥消息。
 */
class T0A00_8A00 {
private:
    int e;               // RSA 公钥 {e,n} 中的 e
    std::vector<uint8_t> n; // RSA 公钥 {e,n} 中的 n，长度为 128

public:
    // Getter 方法
    int getE() const;
    const std::vector<uint8_t>& getN() const;

    // Setter 方法
    void setE(int eVal);
    void setN(const std::vector<uint8_t>& nVal);

    /**
     * 将对象转换为字符串表示，用于调试和日志记录。
     * @return 对象的字符串表示
     */
    std::string toString() const;
};

#endif // T0A00_8A00_H
