#ifndef T1005_H
#define T1005_H

#include <cstdint>
#include <string>

// 表示终端上传乘客流量的类
class T1005 {
public:
    // 构造函数
    T1005();

    // Getter 和 Setter 方法
    std::string getStartTime() const;
    void setStartTime(const std::string& time);

    std::string getEndTime() const;
    void setEndTime(const std::string& time);

    uint16_t getGetOnCount() const;
    void setGetOnCount(uint16_t count);

    uint16_t getGetOffCount() const;
    void setGetOffCount(uint16_t count);

private:
    // 起始时间(YYMMDDHHMMSS)，在数据传输时以 6 字节 BCD 码表示
    std::string startTime;
    // 结束时间(YYMMDDHHMMSS)，在数据传输时以 6 字节 BCD 码表示
    std::string endTime;
    // 从起始时间到结束时间的上车人数，占用 2 字节
    uint16_t getOnCount;
    // 从起始时间到结束时间的下车人数，占用 2 字节
    uint16_t getOffCount;
};

#endif // T1005_H
