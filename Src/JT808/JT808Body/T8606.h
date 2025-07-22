#ifndef T8606_H
#define T8606_H

#include <vector>
#include <string>
#include <memory>
#include <chrono>

class T8606 {
public:
    class Line {
    private:
        int id;               // 拐点 ID，占用 4 字节
        int routeId;          // 路段 ID，占用 4 字节
        int latitude;         // 纬度，占用 4 字节
        int longitude;        // 经度，占用 4 字节
        int width;            // 宽度(米)，占用 1 字节
        int attribute;        // 属性，占用 1 字节
        std::shared_ptr<int> upperLimit; // 路段行驶过长阈值(秒)，若区域属性 0 位为 0 则没有该字段，占用 2 字节
        std::shared_ptr<int> lowerLimit; // 路段行驶不足阈值(秒)，若区域属性 0 位为 0 则没有该字段，占用 2 字节
        std::shared_ptr<int> maxSpeed;   // 路段最高速度(公里每小时)，若区域属性 1 位为 0 则没有该字段，占用 2 字节
        std::shared_ptr<int> duration;   // 路段超速持续时间(秒)，若区域属性 1 位为 0 则没有该字段，占用 1 字节
        std::shared_ptr<int> nightMaxSpeed; // 夜间最高速度(公里每小时)，若区域属性 1 位为 0 则没有该字段，占用 2 字节（版本 1）

    public:
        Line();
        Line(int id, int routeId, int latitude, int longitude, int width, int attribute,
             const std::shared_ptr<int>& upperLimit, const std::shared_ptr<int>& lowerLimit,
             const std::shared_ptr<int>& maxSpeed, const std::shared_ptr<int>& duration);
        Line(int id, int routeId, int latitude, int longitude, int width, int attribute,
             const std::shared_ptr<int>& upperLimit, const std::shared_ptr<int>& lowerLimit,
             const std::shared_ptr<int>& maxSpeed, const std::shared_ptr<int>& duration,
             const std::shared_ptr<int>& nightMaxSpeed);
        ~Line() = default;

        // Getter 方法
        int getId() const;
        int getRouteId() const;
        int getLatitude() const;
        int getLongitude() const;
        int getWidth() const;
        int getAttribute() const;
        std::shared_ptr<int> getUpperLimit() const;
        std::shared_ptr<int> getLowerLimit() const;
        std::shared_ptr<int> getMaxSpeed() const;
        std::shared_ptr<int> getDuration() const;
        std::shared_ptr<int> getNightMaxSpeed() const;

        // Setter 方法，支持链式调用
        Line& setId(int newId);
        Line& setRouteId(int newRouteId);
        Line& setLatitude(int newLatitude);
        Line& setLongitude(int newLongitude);
        Line& setWidth(int newWidth);
        Line& setAttribute(int newAttribute);
        Line& setUpperLimit(const std::shared_ptr<int>& newUpperLimit);
        Line& setLowerLimit(const std::shared_ptr<int>& newLowerLimit);
        Line& setMaxSpeed(const std::shared_ptr<int>& newMaxSpeed);
        Line& setDuration(const std::shared_ptr<int>& newDuration);
        Line& setNightMaxSpeed(const std::shared_ptr<int>& newNightMaxSpeed);

        bool attrBit(int i) const;
    };

private:
    int id;                  // 路线 ID，占用 4 字节
    int attribute;           // 路线属性，占用 2 字节
    std::string startTime;   // 起始时间，若区域属性 0 位为 0 则没有该字段，占用 6 字节（BCD 编码）
    std::string endTime;     // 结束时间，若区域属性 0 位为 0 则没有该字段，占用 6 字节（BCD 编码）
    std::vector<Line> items; // 拐点列表，总单元 2 字节
    int nameLength;          // 区域名称长度，版本 1，占用 2 字节
    std::string name;        // 区域名称，版本 1，长度可变，按 2 字节为单位

public:
    T8606();
    ~T8606() = default;

    // Getter 方法
    int getId() const;
    int getAttribute() const;
    std::string getStartTime() const;
    std::string getEndTime() const;
    const std::vector<Line>& getItems() const;
    std::string getName() const;

    // Setter 方法，支持链式调用
    T8606& setId(int newId);
    T8606& setAttribute(int newAttribute);
    T8606& setStartTime(const std::string& newStartTime);
    T8606& setEndTime(const std::string& newEndTime);
    T8606& setItems(const std::vector<Line>& newItems);
    T8606& setName(const std::string& newName);

    // 添加拐点方法
    T8606& addItem(const Line& item);

    bool attrBit(int i) const;
};

#endif // T8606_H
