#ifndef T0201_0500_H
#define T0201_0500_H

#include <chrono>
#include <map>
#include <memory>

using LocalDateTime = std::chrono::system_clock::time_point;

// 版本 -1 和 0 的 T0201_0500 类
class T0201_0500_VersionMinus1And0 {
private:
    int warnBit;
    int statusBit;
    int latitude;
    int longitude;
    int altitude;
    int speed;
    int direction;
    LocalDateTime deviceTime;
    std::map<int, std::shared_ptr<void>> attributes;
    int responseSerialNo;

public:
    // 报警标志相关方法
    int getWarnBit() const;
    T0201_0500_VersionMinus1And0& setWarnBit(int bit);

    // 状态相关方法
    int getStatusBit() const;
    T0201_0500_VersionMinus1And0& setStatusBit(int bit);

    // 纬度相关方法
    int getLatitude() const;
    T0201_0500_VersionMinus1And0& setLatitude(int lat);

    // 经度相关方法
    int getLongitude() const;
    T0201_0500_VersionMinus1And0& setLongitude(int lon);

    // 高程相关方法
    int getAltitude() const;
    T0201_0500_VersionMinus1And0& setAltitude(int alt);

    // 速度相关方法
    int getSpeed() const;
    T0201_0500_VersionMinus1And0& setSpeed(int spd);

    // 方向相关方法
    int getDirection() const;
    T0201_0500_VersionMinus1And0& setDirection(int dir);

    // 时间相关方法
    LocalDateTime getDeviceTime() const;
    T0201_0500_VersionMinus1And0& setDeviceTime(LocalDateTime time);

    // 位置附加信息相关方法
    std::map<int, std::shared_ptr<void>> getAttributes() const;
    T0201_0500_VersionMinus1And0& setAttributes(const std::map<int, std::shared_ptr<void>>& attrs);
    int getAttributeInt(int key) const;
    long getAttributeLong(int key) const;

    // 转换后的经纬度和速度相关方法
    double getLng() const;
    double getLat() const;
    float getSpeedKph() const;

    // 应答流水号相关方法
    int getResponseSerialNo() const;
    T0201_0500_VersionMinus1And0& setResponseSerialNo(int serialNo);
};

// 版本 1 的 T0201_0500 类
class T0201_0500_Version1 {
private:
    int warnBit;
    int statusBit;
    int latitude;
    int longitude;
    int altitude;
    int speed;
    int direction;
    LocalDateTime deviceTime;
    std::map<int, std::shared_ptr<void>> attributes;
    int responseSerialNo;

public:
    // 报警标志相关方法
    int getWarnBit() const;
    T0201_0500_Version1& setWarnBit(int bit);

    // 状态相关方法
    int getStatusBit() const;
    T0201_0500_Version1& setStatusBit(int bit);

    // 纬度相关方法
    int getLatitude() const;
    T0201_0500_Version1& setLatitude(int lat);

    // 经度相关方法
    int getLongitude() const;
    T0201_0500_Version1& setLongitude(int lon);

    // 高程相关方法
    int getAltitude() const;
    T0201_0500_Version1& setAltitude(int alt);

    // 速度相关方法
    int getSpeed() const;
    T0201_0500_Version1& setSpeed(int spd);

    // 方向相关方法
    int getDirection() const;
    T0201_0500_Version1& setDirection(int dir);

    // 时间相关方法
    LocalDateTime getDeviceTime() const;
    T0201_0500_Version1& setDeviceTime(LocalDateTime time);

    // 位置附加信息相关方法
    std::map<int, std::shared_ptr<void>> getAttributes() const;
    T0201_0500_Version1& setAttributes(const std::map<int, std::shared_ptr<void>>& attrs);
    int getAttributeInt(int key) const;
    long getAttributeLong(int key) const;

    // 转换后的经纬度和速度相关方法
    double getLng() const;
    double getLat() const;
    float getSpeedKph() const;

    // 应答流水号相关方法
    int getResponseSerialNo() const;
    T0201_0500_Version1& setResponseSerialNo(int serialNo);
};

#endif // T0201_0500_H
