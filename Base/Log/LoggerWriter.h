#ifndef LoggerWriter_H_
#define LoggerWriter_H_

#include <thread>
#include <memory>
#include <mutex>
#include <list>
#include <condition_variable>

#include "Util/noncopyable.h"
#include "LoggerStream.h"

using namespace std;

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
    shared_ptr<thread> _thread;
    list<LogContext::Ptr> _pending;
    condition_variable _cv;
    mutex _mutex;
    shared_ptr<Logger> _logger;
};

#endif
