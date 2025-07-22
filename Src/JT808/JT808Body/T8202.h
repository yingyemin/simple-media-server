#ifndef T8202_H
#define T8202_H

class T8202 {
private:
    int interval;         // 时间间隔(秒)，占用 2 字节
    int validityPeriod;   // 有效期(秒)，占用 4 字节

public:
    T8202();
    ~T8202();

    // Getter 方法
    int getInterval() const;
    int getValidityPeriod() const;

    // Setter 方法，支持链式调用
    T8202& setInterval(int newInterval);
    T8202& setValidityPeriod(int newValidityPeriod);
};

#endif // T8202_H
