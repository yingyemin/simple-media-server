#ifndef T0107_H
#define T0107_H

#include <string>

// 版本 -1 和 0 的 T0107 类
class T0107_VersionMinus1And0 {
private:
    // 终端类型，占用 2 字节
    int deviceType;
    // 制造商 ID，终端制造商编码，占用 5 字节
    std::string makerId;
    // 终端型号，由制造商自行定义，位数不足时，后补 "0x00"，占用 20 字节
    std::string deviceModel;
    // 终端 ID，由大写字母和数字组成，此终端 ID 由制造商自行定义，位数不足时，后补 "0x00"，占用 7 字节
    std::string deviceId;
    // 终端 SIM 卡 ICCID，以十六进制字符串表示，占用 10 字节
    std::string iccid;
    // 一字节长度
    int hardwareVersionLength;
    // 硬件版本号，长度由 lengthUnit 决定，占用长度根据实际内容而定
    std::string hardwareVersion;
    // 一字节长度
    int firmwareVersionLength;
    // 固件版本号，长度由 lengthUnit 决定，占用长度根据实际内容而定
    std::string firmwareVersion;
    // GNSS 模块属性，占用 1 字节
    int gnssAttribute;
    // 通信模块属性，占用 1 字节
    int networkAttribute;

public:
    // 获取终端类型
    int getDeviceType() const;
    // 设置终端类型
    void setDeviceType(int type);

    // 获取制造商 ID
    std::string getMakerId() const;
    // 设置制造商 ID
    void setMakerId(const std::string& id);

    // 获取终端型号
    std::string getDeviceModel() const;
    // 设置终端型号
    void setDeviceModel(const std::string& model);

    // 获取终端 ID
    std::string getDeviceId() const;
    // 设置终端 ID
    void setDeviceId(const std::string& id);

    // 获取终端 SIM 卡 ICCID
    std::string getIccid() const;
    // 设置终端 SIM 卡 ICCID
    void setIccid(const std::string& iccidValue);

    // 获取硬件版本号
    std::string getHardwareVersion() const;
    // 设置硬件版本号
    void setHardwareVersion(const std::string& version);

    // 获取固件版本号
    std::string getFirmwareVersion() const;
    // 设置固件版本号
    void setFirmwareVersion(const std::string& version);

    // 获取 GNSS 模块属性
    int getGnssAttribute() const;
    // 设置 GNSS 模块属性
    void setGnssAttribute(int attribute);

    // 获取通信模块属性
    int getNetworkAttribute() const;
    // 设置通信模块属性
    void setNetworkAttribute(int attribute);
};

// 版本 1 的 T0107 类
class T0107_Version1 {
private:
    // 终端类型，占用 2 字节
    int deviceType;
    // 制造商 ID，终端制造商编码，占用 5 字节
    std::string makerId;
    // 终端型号，由制造商自行定义，位数不足时，后补 "0x00"，占用 30 字节
    std::string deviceModel;
    // 终端 ID，由大写字母和数字组成，此终端 ID 由制造商自行定义，位数不足时，后补 "0x00"，占用 30 字节
    std::string deviceId;
    // 终端 SIM 卡 ICCID，以十六进制字符串表示，占用 10 字节
    std::string iccid;
    // 一字节长度
    int hardwareVersionLength;
    // 硬件版本号，长度由 lengthUnit 决定，占用长度根据实际内容而定
    std::string hardwareVersion;
    // 一字节长度
    int firmwareVersionLength;
    // 固件版本号，长度由 lengthUnit 决定，占用长度根据实际内容而定
    std::string firmwareVersion;
    // GNSS 模块属性，占用 1 字节
    int gnssAttribute;
    // 通信模块属性，占用 1 字节
    int networkAttribute;

public:
    // 获取终端类型
    int getDeviceType() const;
    // 设置终端类型
    void setDeviceType(int type);

    // 获取制造商 ID
    std::string getMakerId() const;
    // 设置制造商 ID
    void setMakerId(const std::string& id);

    // 获取终端型号
    std::string getDeviceModel() const;
    // 设置终端型号
    void setDeviceModel(const std::string& model);

    // 获取终端 ID
    std::string getDeviceId() const;
    // 设置终端 ID
    void setDeviceId(const std::string& id);

    // 获取终端 SIM 卡 ICCID
    std::string getIccid() const;
    // 设置终端 SIM 卡 ICCID
    void setIccid(const std::string& iccidValue);

    // 获取硬件版本号
    std::string getHardwareVersion() const;
    // 设置硬件版本号
    void setHardwareVersion(const std::string& version);

    // 获取固件版本号
    std::string getFirmwareVersion() const;
    // 设置固件版本号
    void setFirmwareVersion(const std::string& version);

    // 获取 GNSS 模块属性
    int getGnssAttribute() const;
    // 设置 GNSS 模块属性
    void setGnssAttribute(int attribute);

    // 获取通信模块属性
    int getNetworkAttribute() const;
    // 设置通信模块属性
    void setNetworkAttribute(int attribute);
};

#endif // T0107_H
