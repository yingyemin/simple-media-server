#ifndef TimeClock_H
#define TimeClock_H

#include <chrono>
#include <string>

using namespace std;

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
    static string getFmtTime(const char *fmt, time_t time = 0);

private:
    chrono::system_clock::time_point _create;
    chrono::system_clock::time_point _start;
};

#endif