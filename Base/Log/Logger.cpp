#include "Logger.h"
#include "LoggerChannel.h"
#include "LoggerStream.h"
#include "LoggerWriter.h"

Logger::Ptr g_defaultLogger = Logger::instance();

///////////////////Logger///////////////////

Logger::Ptr Logger::instance() 
{ 
    static std::shared_ptr<Logger> s_instance(new Logger(Path::exeName())); 
    return s_instance;
}

Logger::Logger(const string &loggerName)
    :_loggerName(loggerName)
{
}

Logger::~Logger()
{
    _writer.reset();
    LogStream(shared_from_this(), LInfo, __FILE__, __FUNCTION__, __LINE__);
    _channels.clear();
}

void Logger::addChannel(const std::shared_ptr<LogChannel> &channel) {
    _channels[channel->name()] = channel;
}

void Logger::delChannel(const string &name) {
    _channels.erase(name);
}

std::shared_ptr<LogChannel> Logger::getChannel(const string &name) {
    auto it = _channels.find(name);
    if (it == _channels.end()) {
        return nullptr;
    }
    return it->second;
}

void Logger::setWriter(const std::shared_ptr<LogWriter> &writer) {
    _writer = writer;
}

void Logger::write(const LogContext::Ptr &ctx)
{
    if (_writer) {
        _writer->write(ctx);
    } else {
        writeToChannels(ctx);
    }
}

void Logger::setLevel(LogLevel level) {
    _level = level;
    for (auto &chn : _channels) {
        chn.second->setLevel(level);
    }
}

LogLevel Logger::getLevel() 
{
    return _level;
}

void Logger::writeToChannels(const LogContext::Ptr &ctx) {
    for (auto &chn : _channels) {
        chn.second->write(shared_from_this(), ctx);
    }
}

const string &Logger::getName() const {
    return _loggerName;
}
