#ifndef T0100_H
#define T0100_H

#include "Net/Buffer.h"

/**
 * 对应版本 -1 的 T0100 消息类
 */
class T0100VNeg1 {
private:
    uint16_t provinceId;  // 省域 ID，2 字节
    uint16_t cityId;      // 市县域 ID，2 字节
    std::string makerId; // 制造商 ID，5 字节
    std::string deviceModel; // 终端型号，8 字节
    std::string deviceId; // 终端 ID，7 字节
    uint8_t plateColor; // 车牌颜色，1 字节
    std::string plateNo; // 车辆标识，长度不定

public:
    // Getter 方法
    uint16_t getProvinceId() const;
    uint16_t getCityId() const;
    const std::string& getMakerId() const;
    const std::string& getDeviceModel() const;
    const std::string& getDeviceId() const;
    uint8_t getPlateColor() const;
    const std::string& getPlateNo() const;

    // Setter 方法
    void setProvinceId(uint16_t id);
    void setCityId(uint16_t id);
    void setMakerId(const std::string& id);
    void setDeviceModel(const std::string& model);
    void setDeviceId(const std::string& id);
    void setPlateColor(uint8_t color);
    void setPlateNo(const std::string& no);

    int getProtocolVersion() const;
    std::string toString() const;

    /**
     * 从字节流解析数据填充对象
     * @param buffer 字节流
     * @param offset 起始偏移量
     */
    void parse(const char* buffer, size_t offset);

    /**
     * 将对象数据编码为字节流
     * @param buffer 用于存储编码后数据的字节流
     */
    StringBuffer::Ptr encode() const;
};

/**
 * 对应版本 0 的 T0100 消息类
 */
class T0100V0 {
private:
    uint16_t provinceId;  // 省域 ID，2 字节
    uint16_t cityId;      // 市县域 ID，2 字节
    std::string makerId; // 制造商 ID，5 字节
    std::string deviceModel; // 终端型号，20 字节
    std::string deviceId; // 终端 ID，7 字节
    uint8_t plateColor; // 车牌颜色，1 字节
    std::string plateNo; // 车辆标识，长度不定

public:
    // Getter 方法
    uint16_t getProvinceId() const;
    uint16_t getCityId() const;
    const std::string& getMakerId() const;
    const std::string& getDeviceModel() const;
    const std::string& getDeviceId() const;
    uint8_t getPlateColor() const;
    const std::string& getPlateNo() const;

    // Setter 方法
    void setProvinceId(uint16_t id);
    void setCityId(uint16_t id);
    void setMakerId(const std::string& id);
    void setDeviceModel(const std::string& model);
    void setDeviceId(const std::string& id);
    void setPlateColor(uint8_t color);
    void setPlateNo(const std::string& no);

    int getProtocolVersion() const;
    std::string toString() const;

    /**
     * 从字节流解析数据填充对象
     * @param buffer 字节流
     * @param offset 起始偏移量
     */
    void parse(const char* buffer, size_t offset);

    /**
     * 将对象数据编码为字节流
     * @param buffer 用于存储编码后数据的字节流
     */
    StringBuffer::Ptr encode() const;
};

/**
 * 对应版本 1 的 T0100 消息类
 */
class T0100V1 {
private:
    uint16_t provinceId;  // 省域 ID，2 字节
    uint16_t cityId;      // 市县域 ID，2 字节
    std::string makerId; // 制造商 ID，11 字节
    std::string deviceModel; // 终端型号，30 字节
    std::string deviceId; // 终端 ID，30 字节
    uint8_t plateColor; // 车牌颜色，1 字节
    std::string plateNo; // 车辆标识，长度不定

public:
    // Getter 方法
    uint16_t getProvinceId() const;
    uint16_t getCityId() const;
    const std::string& getMakerId() const;
    const std::string& getDeviceModel() const;
    const std::string& getDeviceId() const;
    uint8_t getPlateColor() const;
    const std::string& getPlateNo() const;

    // Setter 方法
    void setProvinceId(uint16_t id);
    void setCityId(uint16_t id);
    void setMakerId(const std::string& id);
    void setDeviceModel(const std::string& model);
    void setDeviceId(const std::string& id);
    void setPlateColor(uint8_t color);
    void setPlateNo(const std::string& no);

    int getProtocolVersion() const;
    std::string toString() const;

    /**
     * 从字节流解析数据填充对象
     * @param buffer 字节流
     * @param offset 起始偏移量
     */
    void parse(const char* buffer, size_t offset);

    /**
     * 将对象数据编码为字节流
     * @param buffer 用于存储编码后数据的字节流
     */
    StringBuffer::Ptr encode() const;
};

#endif // T0100_H