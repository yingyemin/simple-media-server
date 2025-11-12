#include "LoggerChannel.h"
#include "Util/Thread.h"
#include "Util/TimeClock.h"
#include "Util/File.h"

#include "Logger.h"


#include <sys/stat.h>
#if defined(_WIN32)
#include "Util/Util.h"
#include "strptime_win.h"
#include "Util/String.hpp"
#else
#include "Util/String.hpp"
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/syslog.h>
#endif
#include <sys/types.h>

#include <iostream>

using namespace std;

#ifndef _WIN32
#define start_with(x, y) startWith(x, y)
#define end_with(x, y) endWith(x, y)
#endif


#define CLEAR_COLOR "\033[0m"
static const char *LOG_CONST_TABLE[][3] = {
        {"\033[44;37m", "\033[34m", "T"},
        {"\033[42;37m", "\033[32m", "D"},
        {"\033[46;37m", "\033[36m", "I"},
        {"\033[43;37m", "\033[33m", "W"},
        {"\033[41;37m", "\033[31m", "E"}};

static inline const char *getFileName(const char *file) {
    auto pos = strrchr(file, '/');
    
    return pos ? pos + 1 : file;
}

///////////////////ConsoleChannel///////////////////

ConsoleChannel::ConsoleChannel(const string &name, LogLevel level) 
    : LogChannel(name, level) 
{}

ConsoleChannel::~ConsoleChannel() 
{}

void ConsoleChannel::write(const Logger::Ptr &logger, const LogContext::Ptr &ctx) 
{
    if (_level > ctx->_level) {
        return;
    }

    //linux/windows日志启用颜色并显示日志详情
    format(logger, std::cout, ctx);
}

///////////////////SysLogChannel///////////////////
#ifndef _WIN32
SysLogChannel::SysLogChannel(const string &name, LogLevel level) : LogChannel(name, level) {}
SysLogChannel::~SysLogChannel() {}

void SysLogChannel::write(const Logger::Ptr &logger, const LogContext::Ptr &ctx) {
    if (_level > ctx->_level) {
        return;
    }
    static int s_syslog_lev[10] = {LOG_DEBUG, LOG_INFO, LOG_NOTICE, LOG_WARNING, LOG_ERR};

    syslog(s_syslog_lev[ctx->_level], "-> %s %d\r\n", ctx->_file.c_str(), ctx->_line);
    
    syslog(s_syslog_lev[ctx->_level], "## %s %s | %s %s\r\n", printTime(ctx->_tv).data(),
           LOG_CONST_TABLE[ctx->_level][2], ctx->_function.c_str(), ctx->str().c_str());
}
#endif
///////////////////LogChannel///////////////////
LogChannel::LogChannel(const string &name, LogLevel level) 
    : _name(name)
    , _level(level) 
{}

LogChannel::~LogChannel() 
{}

const string &LogChannel::name() const 
{ 
    return _name; 
}

void LogChannel::setLevel(LogLevel level) 
{ 
    _level = level; 
}

std::string LogChannel::printTime(const timeval &tv) 
{
    auto tm = TimeClock::localtime(tv.tv_sec);
    char buf[128];
    snprintf(buf, sizeof(buf), "%d-%02d-%02d %02d:%02d:%02d.%03d",
             tm.tm_year,
             tm.tm_mon,
             tm.tm_mday,
             tm.tm_hour,
             tm.tm_min,
             tm.tm_sec,
             (int) (tv.tv_usec / 1000));
    return buf;
}

void LogChannel::format(const Logger::Ptr &logger, ostream &ost, const LogContext::Ptr &ctx, bool enableColor, bool enableDetail)
{
    if (!enableDetail && ctx->str().empty()) {
        //没有任何信息打印
        return;
    }

    if (enableColor) {
        ost << LOG_CONST_TABLE[ctx->_level][1];
    }

    ost << printTime(ctx->_tv) << " | " << LOG_CONST_TABLE[ctx->_level][2] << " | ";

    if (enableDetail) {
        ost << getpid() << "[" << ctx->_thread_id << "] | " << ctx->_thread_name << " | "
            << ctx->_file << ":" << ctx->_line << " | "<< ctx->_function << " | ";
    }

    ost << ctx->str();

    if (enableColor) {
        ost << CLEAR_COLOR;
    }

    ost << endl;
}

///////////////////FileChannelBase///////////////////
FileChannelBase::FileChannelBase(const string &name, const string &path, LogLevel level) 
    : LogChannel(name, level)
    , _path(path) 
{}

FileChannelBase::~FileChannelBase() {
    close();
}

void FileChannelBase::write(const Logger::Ptr &logger, const LogContext::Ptr &ctx) {
    if (_level > ctx->_level) {
        return;
    }
    if (!_fstream.is_open()) {
        open();
    }
    //打印至文件，不启用颜色
    format(logger, _fstream, ctx, false);
}

bool FileChannelBase::setPath(const string &path) {
    _path = path;
    return open();
}

const string &FileChannelBase::path() const {
    return _path;
}

bool FileChannelBase::open() {
    // Ensure a path was set
    if (_path.empty()) {
        throw runtime_error("Log file path must be set.");
    }
    // Open the file stream
    _fstream.close();

    File::createDir(_path.c_str(),0);

    _fstream.open(_path.c_str(), ios::out | ios::app);
    // Throw on failure
    if (!_fstream.is_open()) {
        return false;
        //throw runtime_error("Failed to open log file: " + _path);
    }
    //打开文件成功
    return true;
}

void FileChannelBase::close() {
    _fstream.close();
}

size_t FileChannelBase::size() {
    return (_fstream << std::flush).tellp();
}

///////////////////FileChannel///////////////////

struct tm getLocalTime(time_t sec) {
    struct tm tm;
#ifdef _WIN32
    localtime_s(&tm, &sec);
#else
    localtime_r(&sec, &tm);
#endif //_WIN32
    return tm;
}

static const auto s_second_per_day = 24 * 60 * 60;
#ifndef _WIN32
static long s_gmtoff = getLocalTime(time(NULL)).tm_gmtoff;
#endif

//根据日志文件名返回GMT UNIX时间戳
static time_t getLogFileTime(const string &full_path){
    auto name1 = getFileName(full_path.data());
    struct tm tm{0};
    string name2 = name1;
    auto start = name2.find(".");
    auto end = name2.find_last_of(".");
    auto name = name2.substr(start+1,end-start-1);
    auto fileName = name.c_str();
    if (!strptime(fileName, "%Y-%m-%d", &tm)) {
        return 0;
    }
    //此函数会把本地时间转换成GMT时间戳
    return mktime(&tm);
}

static uint64_t getDay(time_t second) {
#if defined(_WIN32)
    return (second + getGMTOff()) / s_second_per_day;
#else
    return (second + s_gmtoff) / s_second_per_day;
#endif
}

static string getLogFilePath(const string &dir, const string &prefixName, time_t second, LogLevel level, int32_t index) {
    //time_t s1 = time(nullptr);
    //struct tm *p = localtime(&s1);
    //struct tm *tm = localtime(&second);
    auto tm = TimeClock::localtime(second);
    char buf[128];
    snprintf(buf, sizeof(buf), "%s.%d-%02d-%02d_%02d.log", prefixName.c_str(), tm.tm_year, tm.tm_mon, tm.tm_mday, index);
    
    return dir + buf;
}

FileChannel::~FileChannel() {}

FileChannel::FileChannel(const string &name, const string& dir, const string &prefixName, LogLevel level) 
    : FileChannelBase(name, "", level) 
{
    _dir = dir;
    if (_dir.back() != '/') {
        _dir.append("/");
    }

    _prefixName = prefixName;

    //收集所有日志文件: _log_file_map将存储此文件夹下所有的log,因此即使流媒体重启，也能获取或清除超过max_day的日志
    File::scanDir(_dir, [this](const string &path, bool isDir) -> bool {
        if (!isDir && end_with(path, ".log")) {
            _log_file_map.emplace(path);
        }
        return true;
    }, false);

    //获取今天日志文件的最大index号
    auto log_name_prefix = TimeClock::getFmtTime("%Y-%m-%d_");
    for (auto it = _log_file_map.begin(); it != _log_file_map.end(); ++it) {
        auto name1 = getFileName(it->data());
        string name= name1;
        //InfoL<<"file name :"<<name<<endl;
        //筛选出今天所有的日志文件
        //if (start_with(name, log_name_prefix)) {
        if(name.find(log_name_prefix) != string::npos){
            int tm_mday;  // day of the month - [1, 31]
            int tm_mon;   // months since January - [0, 11]
            int tm_year;  // years since 1900
            //今天第几个文件
            
            uint32_t index;
            int count = sscanf(name1, (_prefixName + ".%d-%02d-%02d_%d.log").c_str(), &tm_year, &tm_mon, &tm_mday, &index);
            if (count == 4) {
                _index = index >= _index ? index : _index;
            }
        }
    }
}

void FileChannel::write(const Logger::Ptr &logger, const LogContext::Ptr &ctx) {
   //GMT UNIX时间戳
    time_t second = ctx->_tv.tv_sec;
    //这条日志所在第几天
    auto day = getDay(second);
    if ((int64_t) day != _last_day) {
        if (_last_day != -1) {
            //重置日志index
            //_index = 0;
            _index=0;
        }
        //这条日志是新的一天，记录这一天
        _last_day = day;
        //获取日志当天对应的文件，每天可能有多个日志切片文件
        changeFile(second, ctx);
    } else {
        //检查该天日志是否需要重新分片
        checkSize(second, ctx);
    }

    //写日志
    if (_canWrite) {
        FileChannelBase::write(logger, ctx);
    }
}

void FileChannel::clean() {
    //获取今天是第几天
    auto today = getDay(time(NULL));
    // InfoL<<"today : "<<today<<endl;
    // InfoL<<"log map size:"<<_log_file_map.size()<<endl;
    //遍历所有日志文件，删除老日志
    size_t count = 0;
    for (auto it = _log_file_map.begin(); it != _log_file_map.end();) {
        auto log_name = getFileName(it->data());
        auto day = getDay(getLogFileTime(it->data()));
        //InfoL<<"delete file:"<<"file day :  "<<day<<"; today : "<<today<<"; max day: "<<_log_max_day<<endl;
        if (today < day + _log_max_day) {
            //这个日志文件距今不超过_log_max_day天
            if(startWith(log_name, _prefixName)){
                count++;
            }
            ++it;
        }else{
            //这个文件距今超过了_log_max_day天，则删除文件
            File::deleteFile(it->data());
            //删除这条记录
            it = _log_file_map.erase(it);
        }
    }

    //只删除sms log文件
    for (auto it = _log_file_map.begin();it != _log_file_map.end();) {
        auto log_name = getFileName(it->data());
        if(count > _log_max_count){
            bool is = startWith(log_name,_prefixName);
            if(!is || *it == path()){
                ++it;
            }else if(is && *it == path()){
                ++it;
                count--;
            }else{
                File::deleteFile(it->data());
                it = _log_file_map.erase(it);
                count--;
            }
        }else{
            break;
        }
    }
}

void FileChannel::checkSize(time_t second, const LogContext::Ptr &ctx) {
    //每60秒检查一下文件大小，防止频繁flush日志文件
    if (second - _last_check_time > 60) {
        if (FileChannelBase::size() > _log_max_size * 1024 * 1024) {
            changeFile(second,ctx);
        }
        _last_check_time = second;
    }
}

void FileChannel::changeFile(time_t second, const LogContext::Ptr &ctx) {
    size_t index = _index;
    _index;

    auto log_file = getLogFilePath(_dir, _prefixName, second, ctx->_level,index);
    //记录所有的日志文件，以便后续删除老的日志
    _log_file_map.emplace(log_file);
    //打开新的日志文件
    _canWrite = setPath(log_file);
    if (!_canWrite) {
        logError << "Failed to open log file: " << _path;
    }
    //尝试删除过期的日志文件
    clean();
}

void FileChannel::setMaxDay(int max_day) {
    _log_max_day = max_day > 1 ? max_day : _log_max_day;
}

void FileChannel::setFileMaxSize(size_t max_size) {
    _log_max_size = max_size > 1 ? max_size : _log_max_size;
}

void FileChannel::setFileMaxCount(size_t max_count) {
    _log_max_count = max_count > 1 ? max_count : _log_max_count;
}

