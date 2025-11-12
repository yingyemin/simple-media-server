#ifndef LoggerChannel_H_
#define LoggerChannel_H_

#include <fstream>
#include <set>

#include "Util/Path.h"
#include "Util/noncopyable.h"
#include "LoggerStream.h"
#include <string.h>
//using namespace std;

///////////////////LogChannel///////////////////
/**
 * 日志通道
 */
class LogChannel : public noncopyable{
public:
    LogChannel(const std::string& name, LogLevel level = LTrace);
    virtual ~LogChannel();
    virtual void write(const std::shared_ptr<Logger>& logger,const LogContext::Ptr & ctx) = 0;
    const std::string &name() const ;
    void setLevel(LogLevel level);
    static std::string printTime(const timeval &tv);
protected:
    /**
    * 打印日志至输出流
    * @param ost 输出流
    * @param enableColor 是否启用颜色
    * @param enableDetail 是否打印细节(函数名、源码文件名、源码行)
    */
    virtual void format(const std::shared_ptr<Logger> &logger, std::ostream &ost, const LogContext::Ptr & ctx, bool enableColor = true, bool enableDetail = true);
protected:
    std::string _name;
    LogLevel _level;
};

/**
 * 输出日志至终端，支持输出日志至android logcat
 */
class ConsoleChannel : public LogChannel {
public:
    ConsoleChannel(const std::string &name = "ConsoleChannel" , LogLevel level = LTrace) ;
    ~ConsoleChannel();
    void write(const std::shared_ptr<Logger>& logger , const LogContext::Ptr &logContext) override;
};

/**
 * 输出日志至文件
 */
class FileChannelBase : public LogChannel {
public:
    FileChannelBase(const std::string &name = "FileChannelBase",const std::string &path = Path::exePath() + ".log", LogLevel level = LTrace);
    ~FileChannelBase();

    void write(const std::shared_ptr<Logger> &logger , const LogContext::Ptr &ctx) override;
    bool setPath(const std::string &path);
    const std::string &path() const;
protected:
    virtual bool open();
    virtual void close();
    virtual size_t size();
protected:
    std::ofstream _fstream;
    std::string _path;
};

/**
 * 自动清理的日志文件通道
 * 默认最多保存30天的日志
 */
class FileChannel : public FileChannelBase {
public:
    FileChannel(const std::string &name = "FileChannel",const std::string &dir = Path::exeDir() + "log/", 
                                const std::string &prefixName = "SimpleMediaServer", LogLevel level = LTrace);
    ~FileChannel() override;

    /**
     * 写日志时才会触发新建日志文件或者删除老的日志文件
     * @param logger
     * @param stream
     */
    void write(const std::shared_ptr<Logger> &logger , const LogContext::Ptr &ctx) override;

    /**
     * 设置日志最大保存天数
     * @param max_day
     */
    void setMaxDay(int max_day);

    /**
     * 设置日志切片文件最大大小
     * @param max_size 单位MB
     */
    void setFileMaxSize(size_t max_size);

    /**
     * 设置日志切片文件最大个数
     * @param max_count 个数
     */
    void setFileMaxCount(size_t max_count);

    //string getLogFilePath(const string &dir, time_t second, LogLevel level) ;

private:
    /**
     * 删除老文件
     */
    void clean();
    /**
     * 检查当前日志切片文件大小，如果超过限制，则创建新的日志切片文件
     */
    void checkSize(time_t second, const LogContext::Ptr &ctx);

    /**
     * 创建并切换到下一个日志切片文件
     */
    void changeFile(time_t second, const LogContext::Ptr &ctx);

private:
    bool _canWrite = false;
    std::string _dir;
    std::string _prefixName = "SimpleMediaServer";
    int64_t _last_day = -1;
    std::set<std::string> _log_file_map;
    //日志最大存活时间为30天
    int _log_max_day = 30;
    //单个日志切片默认最大为1024MB
    size_t _log_max_size = 1024;
    //最多默认保持100个日志切片文件
    size_t _log_max_count = 100;
    time_t _last_check_time = 0;
    size_t _index = 0;
};

class SysLogChannel : public LogChannel {
public:
    SysLogChannel(const std::string &name = "SysLogChannel" , LogLevel level = LTrace) ;
    ~SysLogChannel();
    void write(const std::shared_ptr<Logger> &logger , const LogContext::Ptr &logContext) override;
};

#endif /* UTIL_LOGGER_H_ */
