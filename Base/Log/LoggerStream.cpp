#include "LoggerStream.h"
#include "Util/Thread.h"
#include "Logger.h"

#include <string.h>

static inline const char *getFileName(const char *file) {
    auto pos = strrchr(file, '/');
    
    return pos ? pos + 1 : file;
}

static inline const char *getFunctionName(const char *func) {
#ifndef _WIN32
    return func;
#else
    auto pos = strrchr(func, ':');
    return pos ? pos + 1 : func;
#endif
}

LogContext::LogContext(LogLevel level, const char *file, const char *function, int line) :
        _level(level),
        _line(line),
        _file(getFileName(file)),
        _function(getFunctionName(function)) 
{
    gettimeofday(&_tv, NULL);
    _thread_name = Thread::getThreadName();
    _thread_id = Thread::getThreadId();
}

///////////////////AsyncLogWriter///////////////////
LogStream::LogStream(const shared_ptr<Logger> &logger, LogLevel level, const char *file, const char *function, int line)
    :_ctx(new LogContext(level, file, function, line))
    ,_logger(logger)
{
}

LogStream::LogStream(const LogStream &that)
    : _ctx(that._ctx)
    , _logger(that._logger) 
{
    const_cast<LogContext::Ptr &>(that._ctx).reset();
}

LogStream::~LogStream() {
    *this << endl;
}

LogStream &LogStream::operator<<(ostream &(*f)(ostream &)) {
    if (!_ctx) {
        return *this;
    }
    _logger->write(_ctx);
    _ctx.reset();
    return *this;
}

void LogStream::clear() {
    _ctx.reset();
}
