#ifndef T0200_H
#define T0200_H

#include <string>
#include <map>
#include <chrono>
#include <memory>

#include "Net/Buffer.h"


// 位置汇报
class AttributeConverter
{
public:
    uint8_t id;
    uint8_t len;
    uint8_t* data;
};

// 版本 -1 和 0 的 T0200 类
class T0200_VersionMinus1And0 {
private:
    // 报警标志，占用 4 字节
    int warnBit;
    // 状态，占用 4 字节
    int statusBit;
    // 纬度，占用 4 字节
    int latitude;
    // 经度，占用 4 字节
    int longitude;
    // 高程(米)，占用 2 字节
    int altitude;
    // 速度(1/10 公里每小时)，占用 2 字节
    int speed;
    // 方向，占用 2 字节
    int direction;
    // 时间(YYMMDDHHMMSS)，BCD, 占用 6 字节
    std::string deviceTime;
    // 位置附加信息
    std::map<int, std::shared_ptr<AttributeConverter>> attributes;

public:
    // 获取报警标志
    int getWarnBit() const;
    // 设置报警标志
    void setWarnBit(int bit);
    // 获取状态
    int getStatusBit() const;
    // 设置状态
    void setStatusBit(int bit);
    // 获取纬度
    int getLatitude() const;
    // 设置纬度
    void setLatitude(int lat);
    // 获取经度
    int getLongitude() const;
    // 设置经度
    void setLongitude(int lon);
    // 获取高程(米)
    int getAltitude() const;
    // 设置高程(米)
    void setAltitude(int alt);
    // 获取速度(1/10 公里每小时)
    int getSpeed() const;
    // 设置速度(1/10 公里每小时)
    void setSpeed(int spd);
    // 获取方向
    int getDirection() const;
    // 设置方向
    void setDirection(int dir);
    // 获取时间
    std::string getDeviceTime() const;
    // 设置时间
    void setDeviceTime(std::string time);
    // 获取位置附加信息
    std::map<int, std::shared_ptr<AttributeConverter>> getAttributes() const;
    // 设置位置附加信息
    void setAttributes(const std::map<int, std::shared_ptr<AttributeConverter>>& attrs);
    // 获取整型位置附加信息
    std::shared_ptr<AttributeConverter> getAttributeInt(int key) const;
    // 获取转换后的经度
    double getLng() const;
    // 获取转换后的纬度
    double getLat() const;
    // 获取速度(公里每小时)
    float getSpeedKph() const;
    
    // 解析数据
    void parse(const uint8_t* data, size_t length);
    // 编码数据
    StringBuffer::Ptr encode() const;
};

class AttributeConverterYue
{
public:
    uint8_t id;
    uint8_t len;
    uint8_t* data;
};

// 版本 1 的 T0200 类
class T0200_Version1 {
private:
    // 报警标志，占用 4 字节
    int warnBit;
    // 状态，占用 4 字节
    int statusBit;
    // 纬度，占用 4 字节
    int latitude;
    // 经度，占用 4 字节
    int longitude;
    // 高程(米)，占用 2 字节
    int altitude;
    // 速度(1/10 公里每小时)，占用 2 字节
    int speed;
    // 方向，占用 2 字节
    int direction;
    // 时间(YYMMDDHHMMSS)，占用 6 字节
    std::string deviceTime;
    // 位置附加信息(粤标)
    std::map<int, std::shared_ptr<AttributeConverterYue>> attributes;

public:
    // 获取报警标志
    int getWarnBit() const;
    // 设置报警标志
    void setWarnBit(int bit);
    // 获取状态
    int getStatusBit() const;
    // 设置状态
    void setStatusBit(int bit);
    // 获取纬度
    int getLatitude() const;
    // 设置纬度
    void setLatitude(int lat);
    // 获取经度
    int getLongitude() const;
    // 设置经度
    void setLongitude(int lon);
    // 获取高程(米)
    int getAltitude() const;
    // 设置高程(米)
    void setAltitude(int alt);
    // 获取速度(1/10 公里每小时)
    int getSpeed() const;
    // 设置速度(1/10 公里每小时)
    void setSpeed(int spd);
    // 获取方向
    int getDirection() const;
    // 设置方向
    void setDirection(int dir);
    // 获取时间
    std::string getDeviceTime() const;
    // 设置时间
    void setDeviceTime(std::string time);
    // 获取位置附加信息(粤标)
    std::map<int, std::shared_ptr<AttributeConverterYue>> getAttributes() const;
    // 设置位置附加信息(粤标)
    void setAttributes(const std::map<int, std::shared_ptr<AttributeConverterYue>>& attrs);
    // 获取整型位置附加信息(粤标)
    std::shared_ptr<AttributeConverterYue> getAttributeInt(int key) const;
    // 获取长整型位置附加信息(粤标)
    long getAttributeLong(int key) const;
    // 获取转换后的经度
    double getLng() const;
    // 获取转换后的纬度
    double getLat() const;
    // 获取速度(公里每小时)
    float getSpeedKph() const;
    
    // 解析数据
    void parse(const uint8_t* data, size_t length);
    // 编码数据
    StringBuffer::Ptr encode() const;
};

#endif // T0200_H