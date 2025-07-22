#ifndef T8602_H
#define T8602_H

#include <vector>
#include <string>
#include <memory>
#include <chrono>

class T8602 {
public:
    class Rectangle {
    private:
        int id;                // 区域 ID，占用 4 字节
        int attribute;         // 区域属性，占用 2 字节
        int latitudeUL;        // 左上点纬度，占用 4 字节
        int longitudeUL;       // 左上点经度，占用 4 字节
        int latitudeLR;        // 右下点纬度，占用 4 字节
        int longitudeLR;       // 右下点经度，占用 4 字节
        std::string startTime; // 起始时间，若区域属性 0 位为 0 则没有该字段，占用 6 字节（BCD 编码）
        std::string endTime;   // 结束时间，若区域属性 0 位为 0 则没有该字段，占用 6 字节（BCD 编码）
        std::shared_ptr<int> maxSpeed;      // 最高速度(公里每小时)，若区域属性 1 位为 0 则没有该字段，占用 2 字节
        std::shared_ptr<int> duration;      // 超速持续时间(秒)，若区域属性 1 位为 0 则没有该字段，占用 1 字节
        std::shared_ptr<int> nightMaxSpeed; // 夜间最高速度(公里每小时)，若区域属性 1 位为 0 则没有该字段，占用 2 字节（版本 1）
        std::string name;           // 区域名称，版本 1，长度可变，按 2 字节为单位

    public:
        Rectangle();
        Rectangle(int id, int attribute, int latitudeUL, int longitudeUL, int latitudeLR, int longitudeLR,
                  const std::string& startTime, const std::string& endTime,
                  const std::shared_ptr<int>& maxSpeed, const std::shared_ptr<int>& duration);
        Rectangle(int id, int attribute, int latitudeUL, int longitudeUL, int latitudeLR, int longitudeLR,
                  const std::string& startTime, const std::string& endTime,
                  const std::shared_ptr<int>& maxSpeed, const std::shared_ptr<int>& duration,
                  const std::shared_ptr<int>& nightMaxSpeed, const std::string& name);
        ~Rectangle() = default;

        // Getter 方法
        int getId() const;
        int getAttribute() const;
        int getLatitudeUL() const;
        int getLongitudeUL() const;
        int getLatitudeLR() const;
        int getLongitudeLR() const;
        std::string getStartTime() const;
        std::string getEndTime() const;
        std::shared_ptr<int> getMaxSpeed() const;
        std::shared_ptr<int> getDuration() const;
        std::shared_ptr<int> getNightMaxSpeed() const;
        std::string getName() const;

        // Setter 方法，支持链式调用
        Rectangle& setId(int newId);
        Rectangle& setAttribute(int newAttribute);
        Rectangle& setLatitudeUL(int newLatitudeUL);
        Rectangle& setLongitudeUL(int newLongitudeUL);
        Rectangle& setLatitudeLR(int newLatitudeLR);
        Rectangle& setLongitudeLR(int newLongitudeLR);
        Rectangle& setStartTime(const std::string& newStartTime);
        Rectangle& setEndTime(const std::string& newEndTime);
        Rectangle& setMaxSpeed(const std::shared_ptr<int>& newMaxSpeed);
        Rectangle& setDuration(const std::shared_ptr<int>& newDuration);
        Rectangle& setNightMaxSpeed(const std::shared_ptr<int>& newNightMaxSpeed);
        Rectangle& setName(const std::string& newName);

        bool attrBit(int i) const;
    };

private:
    int action;                // 设置属性，占用 1 字节
    std::vector<Rectangle> items; // 区域项，总单元 1 字节表示列表长度

public:
    T8602();
    ~T8602() = default;

    // Getter 方法
    int getAction() const;
    const std::vector<Rectangle>& getItems() const;

    // Setter 方法，支持链式调用
    T8602& setAction(int newAction);
    T8602& setItems(const std::vector<Rectangle>& newItems);
    T8602& addItem(const Rectangle& item);
};

#endif // T8602_H
