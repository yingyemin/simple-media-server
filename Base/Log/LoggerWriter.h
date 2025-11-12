#ifndef LoggerWriter_H_
#define LoggerWriter_H_

#include <thread>
#include <memory>
#include <mutex>
#include <list>
#include <condition_variable>

#include "Util/noncopyable.h"
#include "LoggerStream.h"

// using namespace std;

/**
 * 写日志器
 */
class LogWriter : public noncopyable {
public:
    LogWriter() {}
    virtual ~LogWriter() {}
    virtual void write(const LogContext::Ptr &ctx) = 0;
};

class AsyncLogWriter : public LogWriter {
public:
    AsyncLogWriter();
    ~AsyncLogWriter();
private:
    void run();
    void flushAll();
    void write(const LogContext::Ptr &ctx) override ;
private:
    bool _exit_flag;
    std::shared_ptr<std::thread> _thread;
    std::list<LogContext::Ptr> _pending;
    std::condition_variable _cv;
    std::mutex _mutex;
    std::shared_ptr<Logger> _logger;
};

#endif
