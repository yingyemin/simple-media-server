#include "TimeClock.h"

using namespace std;

static int is_leap_year(time_t year) {
    if (year % 4) return 0;         /* A year not divisible by 4 is not leap. */
    else if (year % 100) return 1;  /* If div by 4 and not 100 is surely leap. */
    else if (year % 400) return 0;  /* If div by 100 *and* 400 is not leap. */
    else return 1;                  /* If div by 100 and not by 400 is leap. */
}

TimeClock::TimeClock()
{
    _create = chrono::system_clock::now();
    _start = chrono::system_clock::now();
}

void TimeClock::start()
{
    _start = chrono::system_clock::now();
}

void TimeClock::update()
{
    _start = chrono::system_clock::now();
}

uint64_t TimeClock::startToNow()
{
    auto now = chrono::system_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(now - _start);
    return duration.count();
}

uint64_t TimeClock::createToNow()
{
    auto now = chrono::system_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(now - _create);
    return duration.count();
}

uint64_t TimeClock::now()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return static_cast<uint64_t>(millis);
}

struct tm TimeClock::localtime(time_t t, time_t tz, int dst)
{
    const time_t secs_min = 60;
    const time_t secs_hour = 3600;
    const time_t secs_day = 3600*24;

    t += 3600*8;
    t -= tz;                            /* Adjust for timezone. */
    t += 3600*dst;                      /* Adjust for daylight time. */
    time_t days = t / secs_day;         /* Days passed since epoch. */
    time_t seconds = t % secs_day;      /* Remaining seconds. */

    tm tmp;

    tmp.tm_isdst = dst;
    tmp.tm_hour = seconds / secs_hour;
    tmp.tm_min = (seconds % secs_hour) / secs_min;
    tmp.tm_sec = (seconds % secs_hour) % secs_min;

    /* 1/1/1970 was a Thursday, that is, day 4 from the POV of the tm structure
     * where sunday = 0, so to calculate the day of the week we have to add 4
     * and take the modulo by 7. */
    tmp.tm_wday = (days+4)%7;

    /* Calculate the current year. */
    tmp.tm_year = 1970;
    while(1) {
        /* Leap years have one day more. */
        time_t days_this_year = 365 + is_leap_year(tmp.tm_year);
        if (days_this_year > days) break;
        days -= days_this_year;
        tmp.tm_year++;
    }
    tmp.tm_yday = days;  /* Number of day of the current year. */
    /* We need to calculate in which month and day of the month we are. To do
     * so we need to skip days according to how many days there are in each
     * month, and adjust for the leap year that has one more day in February. */
    int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    mdays[1] += is_leap_year(tmp.tm_year);

    tmp.tm_mon = 0;
    while(days >= mdays[tmp.tm_mon]) {
        days -= mdays[tmp.tm_mon];
        tmp.tm_mon++;
    }

    tmp.tm_mday = days+1;  /* Add 1 since our 'days' is zero-based. */
    tmp.tm_year -= 1900;   /* Surprisingly tm_year is year-1900. */

    return tmp;
}

string TimeClock::getFmtTime(const char *fmt, time_t time)
{
    if(!time){
        time = ::time(NULL);
    }
    auto tm = localtime(time);
    char buffer[1024];
    auto success = std::strftime(buffer, sizeof(buffer), fmt, &tm);
    if (0 == success)
        return string(fmt);
    return buffer;
}