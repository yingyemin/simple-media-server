#ifndef T8604_H
#define T8604_H

#include <vector>
#include <string>
#include <memory>
#include <chrono>

class T8604 {
public:
    class Point {
    private:
        int latitude;  // 纬度，占用 4 字节
        int longitude; // 经度，占用 4 字节

    public:
        Point();
        ~Point() = default;

        // Getter 方法
        int getLatitude() const;
        int getLongitude() const;

        // Setter 方法，支持链式调用
        Point& setLatitude(int newLatitude);
        Point& setLongitude(int newLongitude);
    };

private:
    int id;                     // 区域 ID，占用 4 字节
    int attribute;              // 区域属性，占用 2 字节
    std::string startTime;      // 起始时间，若区域属性 0 位为 0 则没有该字段，占用 6 字节（BCD 编码）
    std::string endTime;        // 结束时间，若区域属性 0 位为 0 则没有该字段，占用 6 字节（BCD 编码）
    std::shared_ptr<int> maxSpeed;      // 最高速度(公里每小时)，若区域属性 1 位为 0 则没有该字段，占用 2 字节
    std::shared_ptr<int> duration;      // 超速持续时间(秒)，若区域属性 1 位为 0 则没有该字段，占用 1 字节
    std::vector<Point> points;  // 顶点项，总单元 2 字节
    std::shared_ptr<int> nightMaxSpeed; // 夜间最高速度(公里每小时)，若区域属性 1 位为 0 则没有该字段，占用 2 字节（版本 1）
    int nameLength; // 区域名称长度，版本 1，占用 2 字节
    std::string name;           // 区域名称，版本 1，长度可变，按 2 字节为单位

public:
    T8604();
    ~T8604() = default;

    // Getter 方法
    int getId() const;
    int getAttribute() const;
    std::string getStartTime() const;
    std::string getEndTime() const;
    std::shared_ptr<int> getMaxSpeed() const;
    std::shared_ptr<int> getDuration() const;
    const std::vector<Point>& getPoints() const;
    std::shared_ptr<int> getNightMaxSpeed() const;
    std::string getName() const;

    // Setter 方法，支持链式调用
    T8604& setId(int newId);
    T8604& setAttribute(int newAttribute);
    T8604& setStartTime(const std::string& newStartTime);
    T8604& setEndTime(const std::string& newEndTime);
    T8604& setMaxSpeed(const std::shared_ptr<int>& newMaxSpeed);
    T8604& setDuration(const std::shared_ptr<int>& newDuration);
    T8604& setPoints(const std::vector<Point>& newPoints);
    T8604& setNightMaxSpeed(const std::shared_ptr<int>& newNightMaxSpeed);
    T8604& setName(const std::string& newName);

    // 添加顶点方法
    T8604& addPoint(int longitude, int latitude);

    // 判断属性位方法
    bool attrBit(int i) const;
};

#endif // T8604_H
