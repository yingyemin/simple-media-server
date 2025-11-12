#ifndef LOGGER_H_
#define LOGGER_H_

#include <unordered_map>
#include <memory>
#include <string>

#include "Util/noncopyable.h"
#include "LoggerStream.h"
#include "LoggerChannel.h"
#include "LoggerWriter.h"

// using namespace std;

/**
 * 日志类
 */
class Logger : public std::enable_shared_from_this<Logger> , public noncopyable {
public:
    // friend class AsyncLogWriter;
    using Ptr = std::shared_ptr<Logger>;

    /**
     * 获取日志单例
     * @return
     */
    static Logger::Ptr instance();

    Logger(const std::string &loggerName);
    ~Logger();

    /**
     * 添加日志通道，非线程安全的
     * @param channel log通道
     */
    void addChannel(const std::shared_ptr<LogChannel> &channel);

    /**
     * 删除日志通道，非线程安全的
     * @param name log通道名
     */
    void delChannel(const std::string &name);

    /**
     * 获取日志通道，非线程安全的
     * @param name log通道名
     * @return 线程通道
     */
    std::shared_ptr<LogChannel> getChannel(const std::string &name);

    /**
     * 设置写log器，非线程安全的
     * @param writer 写log器
     */
    void setWriter(const std::shared_ptr<LogWriter> &writer);

    /**
     * 设置所有日志通道的log等级
     * @param level log等级
     */
    void setLevel(LogLevel level);

    LogLevel getLevel();

    /**
     * 获取logger名
     * @return logger名
     */
    const std::string &getName() const;

    /**
     * 写日志
     * @param ctx 日志信息
     */
    void write(const std::shared_ptr<LogContext> &ctx);

    /**
     * 写日志到各channel，仅供AsyncLogWriter调用
     * @param ctx 日志信息
     */
    void writeToChannels(const std::shared_ptr<LogContext> &ctx);
    
private:
    LogLevel _level;
    std::unordered_map<std::string, std::shared_ptr<LogChannel> > _channels;
    std::shared_ptr<LogWriter> _writer;
    std::string _loggerName;
};

//可重置默认值
extern Logger::Ptr g_defaultLogger;

#define logTrace LogStream(g_defaultLogger, LTrace, __FILE__,__FUNCTION__, __LINE__)
#define logDebug LogStream(g_defaultLogger,LDebug, __FILE__,__FUNCTION__, __LINE__)
#define logInfo LogStream(g_defaultLogger,LInfo, __FILE__,__FUNCTION__, __LINE__)
#define logWarn LogStream(g_defaultLogger,LWarn,__FILE__, __FUNCTION__, __LINE__)
#define logError LogStream(g_defaultLogger,LError,__FILE__, __FUNCTION__, __LINE__)
#define logWrite(level) LogStream(g_defaultLogger,level,__FILE__, __FUNCTION__, __LINE__)

#endif /* UTIL_LOGGER_H_ */
