#ifndef T0702_H
#define T0702_H

#include <string>

class T0702 {
private:
    int status;               // 状态，占用 1 字节
    std::string dateTime;     // 时间，格式 BCD，占用 6 字节
    int cardStatus;           // IC 卡读取结果，占用 1 字节
    std::string name;         // 驾驶员姓名，长度单位 1 字节（实际长度可变）
    std::string licenseNo;    // 从业资格证编码，占用 20 字节
    std::string institution;  // 从业资格证发证机构名称，长度单位 1 字节（实际长度可变）
    std::string licenseValidPeriod; // 证件有效期，格式 BCD，占用 4 字节
    std::string idCard;       // 驾驶员身份证号，版本 1 时占用 20 字节

public:
    T0702();
    ~T0702();

    // Getter 方法
    int getStatus() const;
    std::string getDateTime() const;
    int getCardStatus() const;
    std::string getName() const;
    std::string getLicenseNo() const;
    std::string getInstitution() const;
    std::string getLicenseValidPeriod() const;
    std::string getIdCard() const;

    // Setter 方法，支持链式调用
    T0702& setStatus(int status);
    T0702& setDateTime(const std::string& dateTime);
    T0702& setCardStatus(int cardStatus);
    T0702& setName(const std::string& name);
    T0702& setLicenseNo(const std::string& licenseNo);
    T0702& setInstitution(const std::string& institution);
    T0702& setLicenseValidPeriod(const std::string& licenseValidPeriod);
    T0702& setIdCard(const std::string& idCard);
};

#endif // T0702_H
