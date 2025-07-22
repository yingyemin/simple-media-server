#ifndef T8600_H
#define T8600_H

#include <vector>
#include <string>
#include <chrono>
#include <memory>

class T8600 {
public:
    class Circle {
    private:
        int id;                  // 区域 ID，占用 4 字节
        int attribute;           // 区域属性，占用 2 字节
        int latitude;            // 中心点纬度，占用 4 字节
        int longitude;           // 中心点经度，占用 4 字节
        int radius;              // 半径(米)，占用 4 字节
        std::string startTime;   // 起始时间，若区域属性 0 位为 0 则没有该字段，占用 6 字节（BCD 编码）
        std::string endTime;     // 结束时间，若区域属性 0 位为 0 则没有该字段，占用 6 字节（BCD 编码）
        std::shared_ptr<int> maxSpeed;     // 最高速度(公里每小时)，若区域属性 1 位为 0 则没有该字段，占用 2 字节
        std::shared_ptr<int> duration;     // 超速持续时间(秒)，若区域属性 1 位为 0 则没有该字段，占用 1 字节
        std::shared_ptr<int> nightMaxSpeed; // 夜间最高速度(公里每小时)，若区域属性 1 位为 0 则没有该字段，占用 2 字节（版本 1）
        std::string name;          // 区域名称，版本 1，长度可变，按 2 字节为单位

    public:
        Circle();
        Circle(int id, int attribute, int latitude, int longitude, int radius, const std::string& startTime, const std::string& endTime, 
               const std::shared_ptr<int>& maxSpeed, const std::shared_ptr<int>& duration);
        Circle(int id, int attribute, int latitude, int longitude, int radius, const std::string& startTime, const std::string& endTime, 
               const std::shared_ptr<int>& maxSpeed, const std::shared_ptr<int>& duration, const std::shared_ptr<int>& nightMaxSpeed, const std::string& name);
        ~Circle();

        // Getter 方法
        int getId() const;
        int getAttribute() const;
        int getLatitude() const;
        int getLongitude() const;
        int getRadius() const;
        std::string getStartTime() const;
        std::string getEndTime() const;
        std::shared_ptr<int> getMaxSpeed() const;
        std::shared_ptr<int> getDuration() const;
        std::shared_ptr<int> getNightMaxSpeed() const;
        std::string getName() const;

        // Setter 方法，支持链式调用
        Circle& setId(int newId);
        Circle& setAttribute(int newAttribute);
        Circle& setLatitude(int newLatitude);
        Circle& setLongitude(int newLongitude);
        Circle& setRadius(int newRadius);
        Circle& setStartTime(const std::string& newStartTime);
        Circle& setEndTime(const std::string& newEndTime);
        Circle& setMaxSpeed(const std::shared_ptr<int>& newMaxSpeed);
        Circle& setDuration(const std::shared_ptr<int>& newDuration);
        Circle& setNightMaxSpeed(const std::shared_ptr<int>& newNightMaxSpeed);
        Circle& setName(const std::string& newName);

        bool attrBit(int i) const;
    };

private:
    int action;                 // 设置属性，占用 1 字节
    std::vector<Circle> items;  // 区域项，总单元 1 字节表示列表长度

public:
    T8600();
    ~T8600();

    // Getter 方法
    int getAction() const;
    const std::vector<Circle>& getItems() const;

    // Setter 方法，支持链式调用
    T8600& setAction(int newAction);
    T8600& setItems(const std::vector<Circle>& newItems);
    T8600& addItem(const Circle& item);
};

#endif // T8600_H
