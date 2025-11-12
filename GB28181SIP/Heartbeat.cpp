#include <fstream>
#if defined(_WIN32)
    #include "Util/Util.h"
#else
    #include <unistd.h>
#endif
#include "Heartbeat.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "Common/MediaSource.h"
#include "Common/Config.h"
#include "EventPoller/EventLoopPool.h"
#include "GB28181Hook.h"

#include <chrono>
#include <thread>

using namespace std;

// 获取内存使用率
float get_mem_usage() {
    std::ifstream file("/proc/meminfo");
    std::string line;
    float mem_total = 0, mem_free = 0;
    while (std::getline(file, line)) {
        if (line.find("MemTotal:") != std::string::npos) {
            std::istringstream iss(line);
            iss >> line >> mem_total;
            break;
        }
    }
    while (std::getline(file, line)) {
        if (line.find("MemFree:") != std::string::npos) {
            std::istringstream iss(line);
            iss >> line >> mem_free;
            break;
        }
    }
    return (mem_total - mem_free) / mem_total * 100;
}

// 获取CPU使用率
float get_cpu_usage() {
    std::ifstream file("/proc/stat");
    std::string line;
    std::getline(file, line);
 
    std::istringstream iss(line);
    std::string token;
    long user = 0, nice = 0, system = 0, idle = 0;
    while (iss >> token) {
        if (token == "cpu") {
            for (int i = 0; i < 4; ++i) {
                iss >> user >> nice >> system >> idle;
            }
            break;
        }
    }
 
    long total = user + nice + system + idle;
    long total_diff = total - (idle + user);  // 用户态和系统态的CPU时间总和
    long total_idle = idle;                   // 初始的空闲时间
 
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    //sleep(1);  // 等待一秒
 
    std::ifstream file2("/proc/stat");
    std::getline(file2, line);
    std::istringstream iss2(line);
    while (iss2 >> token) {
        if (token == "cpu") {
            for (int i = 0; i < 4; ++i) {
                iss2 >> user >> nice >> system >> idle;
            }
            break;
        }
    }
 
    long total2 = user + nice + system + idle;
    long total_diff2 = total2 - (idle + user);
    long total_idle2 = idle;
 
    float usage = (total_diff2 - total_diff) / (total_diff2 - total_diff + total_idle2 - total_idle);
    return usage * 100;
}


Heartbeat::Heartbeat()
{

}

Heartbeat::~Heartbeat()
{

}

void Heartbeat::registerServer()
{
    logDebug << "registerServer";

    RegisterServerInfo info;
    info.ip = Config::instance()->get("LocalIp");
    info.port = Config::instance()->get("Http", "Api", "Api1", "port");

    
    GB28181Hook::instance()->onRegisterServer(info);
}

void Heartbeat::startAsync()
{
    registerServer();

    static int timeMs = Config::instance()->getAndListen([](const json &config){
        timeMs = Config::instance()->get("Util", "heartbeatTime");
    }, "Util", "heartbeatTime");

    if (timeMs == 0) {
        timeMs = 10000;
    }

    weak_ptr<Heartbeat> wSelf = shared_from_this();
    _loop = EventLoopPool::instance()->getLoopByCircle();
    _loop->addTimerTask(timeMs, [wSelf](){
        logTrace << "start";
        if (auto self = wSelf.lock()) {
            self->start();
        }

        return timeMs;
    }, nullptr);
}

void Heartbeat::start()
{
    string url = Config::instance()->get("Hook", "Http", "onKeepAlive");

    if(url.empty()) {
        return ;
    }

    ServerInfo info;
    getDeviceInfo(info);

    static string ip = Config::instance()->getAndListen([](const json &config){
        ip = Config::instance()->get("LocalIp");
    }, "LocalIp");

    static int port = Config::instance()->getAndListen([](const json &config){
        port = Config::instance()->get("Http", "Api", "Api1", "port");
    }, "Http", "Api", "Api1", "port");

    info.ip = ip;
    info.port = port;
    info.memUsage = get_mem_usage();

    GB28181Hook::instance()->onKeepAlive(info);
}

void Heartbeat::getDeviceInfo(ServerInfo& info)
{
    
}