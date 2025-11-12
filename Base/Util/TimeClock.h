#ifndef TimeClock_H
#define TimeClock_H

#include <chrono>
#include <string>

// using namespace std;

class TimeClock
{
public:
    TimeClock();
    ~TimeClock() {}

public:
    void start();
    void update();
    uint64_t startToNow();
    uint64_t createToNow();

public:
    static uint64_t now();
    static uint64_t nowMicro();
    static struct tm localtime(time_t t, time_t tz = 0, int dst = 0);
    static std::string getFmtTime(const char *fmt, time_t time = 0);
    /**
     * @brief 获取 GMT 格式的时间字符串
     * @param time 可选参数，指定的时间戳，默认为当前时间
     * @return GMT 格式的时间字符串，格式为：Mon, Nov 10 2025 08:39:50 GMT
     */
    static std::string getGmtTime(time_t time = 0);

private:
    std::chrono::system_clock::time_point _create;
    std::chrono::system_clock::time_point _start;
};

#endif