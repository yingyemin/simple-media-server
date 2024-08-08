#ifndef LoggerStream_H_
#define LoggerStream_H_

#include <sstream>
#include <sys/time.h>

// #include "Logger.h"
#include <memory>

using namespace std;

typedef enum 
{ 
    LTrace = 0, 
    LDebug, 
    LInfo, 
    LWarn, 
    LError
} LogLevel;

class Logger;

///////////////////LogContext///////////////////
/**
 * 日志上下文
 */
class LogContext : public ostringstream{
    //_file,_function改成string保存，目的是有些情况下，指针可能会失效
    //比如说动态库中打印了一条日志，然后动态库卸载了，那么指向静态数据区的指针就会失效
public:
    using Ptr =  std::shared_ptr<LogContext>;
    LogContext(LogLevel level, const char *file, const char *function, int line);
    ~LogContext() = default;
    LogLevel _level;
    int _thread_id;
    int _line;
    string _file;
    string _function;
    string _thread_name;
    struct timeval _tv;
};

/**
 * 日志上下文捕获器
 */
class LogStream {
public:
    using Ptr = std::shared_ptr<LogStream>;

    LogStream(const shared_ptr<Logger>& logger, LogLevel level, const char *file, const char *function, int line);
    LogStream(const LogStream &that);

    ~LogStream();

    /**
     * 输入std::endl(回车符)立即输出日志
     * @param f std::endl(回车符)
     * @return 自身引用
     */
    LogStream &operator << (ostream &(*f)(ostream &));

    template<typename T>
    LogStream &operator<<(T &&data) {
        if (!_ctx) {
            return *this;
        }
        (*_ctx) << std::forward<T>(data);
        return *this;
    }

    void clear();
private:
    LogContext::Ptr _ctx;
    shared_ptr<Logger> _logger;
};

#endif