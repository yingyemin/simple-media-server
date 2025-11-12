#include "LoggerWriter.h"
#include "Util/Thread.h"
#include "Logger.h"

using namespace std;

///////////////////AsyncLogWriter///////////////////
AsyncLogWriter::AsyncLogWriter() 
    : _exit_flag(false)
{
    _logger = Logger::instance();
    _thread = std::make_shared<thread>([this]() { this->run(); });
}

AsyncLogWriter::~AsyncLogWriter()
{
    _exit_flag = true;
    _cv.notify_one();
    _thread->join();
    flushAll();
}

void AsyncLogWriter::write(const LogContext::Ptr &ctx)
{
    if (ctx->_level < _logger->getLevel()) {
        return ;
    }
    
    lock_guard<mutex> lock(_mutex);
    _pending.emplace_back(ctx);
    _cv.notify_one();
}

void AsyncLogWriter::run()
{
    Thread::setThreadName("async log");
    while (!_exit_flag) {
        {
            std::unique_lock<std::mutex> lk(_mutex);
            _cv.wait(lk, [this]{ return !_pending.empty() || _exit_flag; });
        }

        flushAll();
    }
}

void AsyncLogWriter::flushAll()
{
    list<LogContext::Ptr> tmp;
    {
        lock_guard<mutex> lock(_mutex);
        tmp.swap(_pending);
    }

    for (auto ctx : tmp) {
        _logger->writeToChannels(ctx);
    }

}
