#include "Log/Logger.h"
#include "EventLoopPool.h"
#include "WorkPoller/WorkLoopPool.h"
#include "Util/Thread.h"
#include "Ssl/TlsContext.h"
#include "Common/Config.h"
#include "gb28181Sip/GB28181TcpClient.h"
#include "gb28181Sip/GB28181UdpClient.h"
#include "gb28181Sip/GB28181SIPServer.h"

#include <unistd.h>
#include <string.h>
#include <sys/resource.h>

using namespace std;

// static unordered_map<string, int> options {
//     "-c" : 1
// };

void setFileLimits()
{
    struct rlimit limitOld, limitNew;

    if (getrlimit(RLIMIT_NOFILE, &limitOld)==0) {
        limitNew.rlim_cur = limitNew.rlim_max = RLIM_INFINITY;
        
        if (setrlimit(RLIMIT_NOFILE, &limitNew)!=0) {
            limitNew.rlim_cur = limitNew.rlim_max = limitOld.rlim_max;
            setrlimit(RLIMIT_NOFILE, &limitNew);
        }
        logInfo << "文件最大描述符个数设置为:" << limitNew.rlim_cur;
    }
}

int main(int argc, char** argv)
{
    Thread::setThreadName("SSS-main");

    string configPath = "./gb28181Sip.json";
    for (int i = 0; i < argc; ++i) {
        if (!strcmp(argv[i], "-c")) {
            configPath = argv[++i];
        }
    }
    // 多线程开启epoll
    EventLoopPool::instance()->init(0, true, true);
    WorkLoopPool::instance()->init(0, true, true);

    // config
    Config::instance()->load(configPath);
    auto configJson = Config::instance()->getConfig();

    int consoleLog = Config::instance()->get("Log", "console");
    if (consoleLog) {
        Logger::instance()->addChannel(std::make_shared<ConsoleChannel>("ConsoleChannel", LTrace));
    }
    auto fileChannel = std::make_shared<FileChannel>("FileChannel", Path::exeDir() + "log/", LTrace);

    //日志最多保存天数
    fileChannel->setMaxDay(Config::instance()->get("Log", "maxDay"));

    //配置日志最大保留个数和单个日志切片大小
    size_t ilogmaxsize = Config::instance()->get("Log", "maxSize");
    size_t ilogmaxcount =  Config::instance()->get("Log", "maxSize");
    fileChannel->setFileMaxCount(ilogmaxcount);
    fileChannel->setFileMaxSize(ilogmaxsize);

    Logger::instance()->addChannel(fileChannel);
    Logger::instance()->setWriter(std::make_shared<AsyncLogWriter>());

    static int logLevel = Config::instance()->getAndListen([](const json &config){
        logLevel = config["Log"]["logLevel"];
        Logger::instance()->setLevel((LogLevel)logLevel);
    }, "Log", "logLevel");

    Logger::instance()->setLevel((LogLevel)logLevel);

    setFileLimits();

    auto sslKey = Config::instance()->get("Ssl", "key");
    auto sslCrt = Config::instance()->get("Ssl", "cert");
    TlsContext::setKeyFile(sslKey, sslCrt);

    auto gb28181SipConfigVec = configJson["SipServer"];
    for (auto server : gb28181SipConfigVec.items()) {
        string serverId = server.key();
        auto gb28181SipConfig = server.value();

        if (!gb28181SipConfig.is_object()) {
            continue;
        }

        const string ip = gb28181SipConfig["ip"];
        int port = gb28181SipConfig["port"];
        int count = gb28181SipConfig["threads"];
        int sockType = gb28181SipConfig["sockType"];

        logInfo << "start gb28181 sip server, port: " << port;
        if (port) {
            GB28181SIPServer::instance()->start(ip, port, count, sockType);
            // RtcServer::Instance().Start(EventLoopPool::instance()->getLoopByCircle(), port, "0.0.0.0");
        }
        // logInfo << "start rtsps server, sslPort: " << sslPort;
        // if (sslPort) {
        //     RtspServer::instance()->start(ip, sslPort, count);
        // }
    }

    // shared_ptr<GB28181Client> client(new GB28181TcpClient());
    // client->start();
    
    while (true) {
    //     // TODO 可做一些巡检工作
        sleep(5);
    //     if (!client->isRegister()) {
    //         client->gbRegister(nullptr);
    //     }
    }

    return 0;
}
