#ifdef ENABLE_RTSP
#include "Rtsp/RtspServer.h"
#include "Rtsp/RtspClient.h"
#endif

#if defined(ENABLE_HTTP) || defined(ENABLE_API) || defined(ENABLE_HLS)
#include "Http/HttpServer.h"
#include "Http/HttpClientApi.h"
#endif

#if defined(ENABLE_GB28181) || defined(ENABLE_EHOME2) || defined(ENABLE_EHOME5)
#include "GB28181/GB28181Server.h"
#endif

#ifdef ENABLE_EHOME2
#include "Ehome2/Ehome2Server.h"
#endif

#ifdef ENABLE_EHOME5
#include "Ehome5/Ehome5Server.h"
#endif

#ifdef ENABLE_RTMP
#include "Rtmp/RtmpServer.h"
#include "Rtmp/RtmpClient.h"
#endif

#ifdef ENABLE_WEBRTC
#include "Webrtc/WebrtcServer.h"
#include "Webrtc/WebrtcClient.h"
#endif

#ifdef ENABLE_SRT
#include "Srt/SrtServer.h"
#endif

#ifdef ENABLE_SRTCUSTOM
#include "SrtCustom/SrtServer.h"
#endif

#ifdef ENABLE_JT1078
#include "JT1078/JT1078Server.h"
#endif

#if defined(ENABLE_GB28181) || defined(ENABLE_EHOME2) || defined(ENABLE_EHOME5) || defined(ENABLE_RTSP) || defined(ENABLE_WEBRTC) || defined(ENABLE_RTP)
#include "Rtp/RtpServer.h"
#endif

#ifdef ENABLE_OPENSSL
#include "Ssl/TlsContext.h"
#endif

#ifdef ENABLE_HOOK
#include "Hook/MediaHook.h"
#endif

#ifdef ENABLE_RECORD
#include "Record/RecordReader.h"
#endif

#if defined(ENABLE_JT808)
#include "JT808/JT808Server.h"
#endif

#ifdef ENABLE_API
#include "Api/HttpApi.h"
#include "Api/RtmpApi.h"
#include "Api/RtspApi.h"
#include "Api/GB28181Api.h"
#include "Api/WebrtcApi.h"
#include "Api/HttpStreamApi.h"
#include "Api/RecordApi.h"
#include "Api/TestApi.h"
#include "Api/JT1078Api.h"
#include "Api/FfmpegApi.h"
#include "Api/HookApi.h"
#include "Api/RtpApi.h"
#include "Api/WebsocketApi.h"
#include "Api/AdminWebsocketApi.h"
#include "Api/BatchOperationApi.h"
#include "Api/ConfigTemplateApi.h"
#include "Api/SrtApi.h"
#include "Api/VodApi.h"
#include "Api/Ehome2Api.h"
#include "Api/JT808Api.h"
#endif

#include "Codec/AacTrack.h"
#include "Codec/AV1Track.h"
#include "Codec/Mp3Track.h"
#include "Codec/G711Track.h"
#include "Codec/AdpcmaTrack.h"
#include "Codec/OpusTrack.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"
#include "Codec/H266Track.h"
#include "Codec/VP8Track.h"
#include "Codec/VP9Track.h"

#include "Codec/AacFrame.h"
#include "Codec/AV1Frame.h"
#include "Codec/H264Frame.h"
#include "Codec/H265Frame.h"
#include "Codec/H266Frame.h"
#include "Codec/VP8Frame.h"
#include "Codec/VP9Frame.h"

#include "Log/Logger.h"
#include "EventLoopPool.h"
#include "WorkPoller/WorkLoopPool.h"
#include "Common/Config.h"
#include "Util/Thread.h"
#include "Common/Heartbeat.h"

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

void setCoreLimits()
{
    struct rlimit limitOld, limitNew;

    if (getrlimit(RLIMIT_CORE, &limitOld)==0) {
        limitNew.rlim_cur = limitNew.rlim_max = RLIM_INFINITY;
        if (setrlimit(RLIMIT_CORE, &limitNew)!=0) {
            limitNew.rlim_cur = limitNew.rlim_max = limitOld.rlim_max;
            setrlimit(RLIMIT_CORE, &limitNew);
        }
        logInfo << "core文件大小设置为:" << limitNew.rlim_cur;
    }
}

int main(int argc, char** argv)
{
    Thread::setThreadName("SMS-main");

    string configPath = "./server.json";
    for (int i = 0; i < argc; ++i) {
        if (!strcmp(argv[i], "-c")) {
            configPath = argv[++i];
        }
    }
    // 多线程开启epoll
    EventLoopPool::instance()->init(0, true, true);
    WorkLoopPool::instance()->init(0, true, true);
#ifdef ENABLE_SRT
    SrtEventLoopPool::instance()->init(1, true, true);
#endif

    // config
    Config::instance()->load(configPath);
    auto configJson = Config::instance()->getConfig();

    int consoleLog = Config::instance()->get("Log", "console");
    if (consoleLog) {
        Logger::instance()->addChannel(std::make_shared<ConsoleChannel>("ConsoleChannel", LTrace));
    }

    string prefixname = Config::instance()->get("Log", "prefixname", "", "", "SimpleMediaServer");
    auto fileChannel = std::make_shared<FileChannel>("FileChannel", Path::exeDir() + "log/", prefixname, LTrace);

    //日志最多保存天数
    static int ilogmaxday = Config::instance()->getAndListen([fileChannel](const json &config){
        ilogmaxday = Config::instance()->get("Log", "maxDay");
        fileChannel->setMaxDay(ilogmaxday);
    }, "Log", "maxDay");
    fileChannel->setMaxDay(ilogmaxday);

    //配置日志最大保留个数和单个日志切片大小
    static int ilogmaxsize = Config::instance()->getAndListen([fileChannel](const json &config){
        ilogmaxsize = Config::instance()->get("Log", "maxSize");
        fileChannel->setFileMaxSize(ilogmaxsize);
    }, "Log", "maxSize");
    fileChannel->setFileMaxSize(ilogmaxsize);

    static int ilogmaxcount = Config::instance()->getAndListen([fileChannel](const json &config){
        ilogmaxcount = Config::instance()->get("Log", "maxCount");
        fileChannel->setFileMaxCount(ilogmaxcount);
    }, "Log", "maxCount");
    fileChannel->setFileMaxCount(ilogmaxcount);

    Logger::instance()->addChannel(fileChannel);
    Logger::instance()->setWriter(std::make_shared<AsyncLogWriter>());

    static int logLevel = Config::instance()->getAndListen([](const json &config){
        logLevel = config["Log"]["logLevel"];
        Logger::instance()->setLevel((LogLevel)logLevel);
    }, "Log", "logLevel");

    Logger::instance()->setLevel((LogLevel)logLevel);

    setFileLimits();
    setCoreLimits();

#ifdef ENABLE_OPENSSL
    auto sslKey = Config::instance()->get("Ssl", "key");
    auto sslCrt = Config::instance()->get("Ssl", "cert");
    TlsContext::setKeyFile(sslKey, sslCrt);
#endif

#ifdef ENABLE_HOOK
    MediaHook::instance()->init();
    HookManager::instance()->setOnHookReportByHttp(HttpClientApi::reportByHttp);
#endif

#ifdef ENABLE_RECORD
    RecordReader::init();
#endif

#ifdef ENABLE_API
#if defined(ENABLE_HTTP) || defined(ENABLE_API) || defined(ENABLE_HLS)
    HttpApi::initApi();
    WebsocketApi::initApi();
    AdminWebsocketApi::initApi();
    BatchOperationApi::initApi();
    ConfigTemplateApi::initApi();
    HttpStreamApi::initApi();
#endif
#ifdef ENABLE_HOOK
    HookApi::initApi();
#endif
#ifdef ENABLE_RTMP
    RtmpApi::initApi();
#endif
#ifdef ENABLE_RTSP
    RtspApi::initApi();
#endif
#ifdef ENABLE_GB28181
    GB28181Api::initApi();
#endif
#ifdef ENABLE_RTP
    RtpApi::initApi();
#endif
#ifdef ENABLE_WEBRTC
    WebrtcApi::initApi();
#endif
#ifdef ENABLE_RECORD
    RecordApi::initApi();
#endif
#ifdef ENABLE_API
    TestApi::initApi();
#endif
#ifdef ENABLE_JT1078
    JT1078Api::initApi();
#endif
#ifdef ENABLE_FFMPEG
    FfmpegApi::initApi();
#endif
#ifdef ENABLE_SRT
    SrtApi::initApi();
#endif
#ifdef ENABLE_EHOME2
    Ehome2Api::initApi();
#endif
#ifdef ENABLE_JT808
    JT808Api::initApi();
#endif

    VodApi::initApi();
#endif

    AacTrack::registerTrackInfo();
    AV1Track::registerTrackInfo();
    Mp3Track::registerTrackInfo();
    G711aTrack::registerTrackInfo();
    G711uTrack::registerTrackInfo();
    AdpcmaTrack::registerTrackInfo();
    OpusTrack::registerTrackInfo();
    H264Track::registerTrackInfo();
    H265Track::registerTrackInfo();
    H266Track::registerTrackInfo();
    VP8Track::registerTrackInfo();
    VP9Track::registerTrackInfo();

    AacFrame::registerFrame();
    AV1Frame::registerFrame();
    H264Frame::registerFrame();
    H265Frame::registerFrame();
    H266Frame::registerFrame();
    VP8Frame::registerFrame();
    VP9Frame::registerFrame();

    Heartbeat::Ptr beat = make_shared<Heartbeat>();
    beat->startAsync();

#ifdef ENABLE_RTSP
    // 开启RTSP SERVER
    // 参数需要改成从配置读取
    RtspClient::init();
    auto rtspConfigVec = configJson["Rtsp"]["Server"];
    for (auto server : rtspConfigVec.items()) {
        string serverId = server.key();
        auto rtspConfig = server.value();

        if (!rtspConfig.is_object()) {
            continue;
        }

        const string ip = rtspConfig["ip"];
        int port = rtspConfig["port"];
        int sslPort = rtspConfig["sslPort"];
        int count = rtspConfig["threads"];

        logInfo << "start rtsp server, port: " << port;
        if (port) {
            RtspServer::instance()->start(ip, port, count);
        }
        logInfo << "start rtsps server, sslPort: " << sslPort;
        if (sslPort) {
            RtspServer::instance()->start(ip, sslPort, count, true);
        }
    }
#endif

#ifdef ENABLE_RTMP
    RtmpClient::init();
    auto rtmpConfigVec = configJson["Rtmp"]["Server"];
    for (auto server : rtmpConfigVec.items()) {
        string serverId = server.key();
        auto rtmpConfig = server.value();

        if (!rtmpConfig.is_object()) {
            continue;
        }

        const string ip = rtmpConfig["ip"];
        int port = rtmpConfig["port"];
        int count = rtmpConfig["threads"];

        logInfo << "start rtmp server, port: " << port;
        if (port) {
            RtmpServer::instance()->start(ip, port, count);
        }
        // logInfo << "start rtsps server, sslPort: " << sslPort;
        // if (sslPort) {
        //     RtspServer::instance()->start(ip, sslPort, count);
        // }
    }
#endif

#if defined(ENABLE_GB28181) || defined(ENABLE_EHOME2) || defined(ENABLE_EHOME5)
    auto GB28181ConfigVec = configJson["GB28181"]["Server"];
    for (auto server : GB28181ConfigVec.items()) {
        string serverId = server.key();
        auto GB28181Config = server.value();

        if (!GB28181Config.is_object()) {
            continue;
        }

        const string ip = GB28181Config["ip"];
        int port = GB28181Config["port"];
        int count = GB28181Config["threads"];
        int sockType = GB28181Config["sockType"];

        logInfo << "start gb28181 server, port: " << port;
        if (port) {
            GB28181Server::instance()->start(ip, port, count, sockType);
        }
        // logInfo << "start rtsps server, sslPort: " << sslPort;
        // if (sslPort) {
        //     RtspServer::instance()->start(ip, sslPort, count);
        // }
    }
#endif

#if defined(ENABLE_GB28181) || defined(ENABLE_EHOME2) || defined(ENABLE_EHOME5) || defined(ENABLE_RTSP) || defined(ENABLE_WEBRTC) || defined(ENABLE_RTP)
    auto rtpConfigVec = configJson["Rtp"]["Server"];
    for (auto server : rtpConfigVec.items()) {
        string serverId = server.key();
        auto rtpConfig = server.value();

        if (!rtpConfig.is_object()) {
            continue;
        }

        const string ip = rtpConfig["ip"];
        int port = rtpConfig["port"];
        int count = rtpConfig["threads"];
        int sockType = rtpConfig["sockType"];

        logInfo << "start rtp server, port: " << port;
        if (port) {
            RtpServer::instance()->start(ip, port, count, sockType);
        }
        // logInfo << "start rtsps server, sslPort: " << sslPort;
        // if (sslPort) {
        //     RtspServer::instance()->start(ip, sslPort, count);
        // }
    }
#endif

#ifdef ENABLE_EHOME2
    auto ehome2ConfigVec = configJson["Ehome2"]["Server"];
    for (auto server : ehome2ConfigVec.items()) {
        string serverId = server.key();
        auto ehomeConfig = server.value();

        if (!ehomeConfig.is_object()) {
            continue;
        }

        const string ip = ehomeConfig["ip"];
        int port = ehomeConfig["port"];
        int count = ehomeConfig["threads"];
        int sockType = ehomeConfig["sockType"];

        logInfo << "start ehome2 server, port: " << port;
        if (port) {
            Ehome2Server::instance()->start(ip, port, count, sockType);
        }
        // logInfo << "start rtsps server, sslPort: " << sslPort;
        // if (sslPort) {
        //     RtspServer::instance()->start(ip, sslPort, count);
        // }
    }
#endif

#ifdef ENABLE_EHOME5
    auto ehome5ConfigVec = configJson["Ehome5"]["Server"];
    for (auto server : ehome5ConfigVec.items()) {
        string serverId = server.key();
        auto ehomeConfig = server.value();

        if (!ehomeConfig.is_object()) {
            continue;
        }

        const string ip = ehomeConfig["ip"];
        int port = ehomeConfig["port"];
        int count = ehomeConfig["threads"];
        int sockType = ehomeConfig["sockType"];

        logInfo << "start ehome5 server, port: " << port;
        if (port) {
            Ehome5Server::instance()->start(ip, port, count, sockType);
        }
        // logInfo << "start rtsps server, sslPort: " << sslPort;
        // if (sslPort) {
        //     RtspServer::instance()->start(ip, sslPort, count);
        // }
    }
#endif

#ifdef ENABLE_JT1078
    auto jt1078ConfigVec = configJson["JT1078"]["Server"];
    int maxPort = jt1078ConfigVec.value("portMax", 0);
    int minPort = jt1078ConfigVec.value("portMin", 0);
    JT1078Server::instance()->setPortRange(minPort, maxPort);

    for (auto server : jt1078ConfigVec.items()) {
        string serverId = server.key();
        auto jt1078Config = server.value();

        if (!jt1078Config.is_object()) {
            continue;
        }

        const string ip = jt1078Config["ip"];
        int port = jt1078Config["port"];
        int count = jt1078Config["threads"];
        int isTalk = jt1078Config.value("isTalk", 0);

        if (isTalk) {
            logInfo << "start jt1078 talk server, port: " << port;
        } else {
            logInfo << "start jt1078 server, port: " << port;
        }
        if (port) {
            JT1078Server::instance()->start(ip, port, count, isTalk);
        }
        // logInfo << "start 1078 server, sslPort: " << sslPort;
        // if (sslPort) {
        //     JT1078Server::instance()->start(ip, sslPort, count, true);
        // }
    }
#endif

#if defined(ENABLE_HTTP) || defined(ENABLE_API) || defined(ENABLE_HLS)
    auto websocketConfigVec = configJson["Websocket"]["Server"];
    for (auto server : websocketConfigVec.items()) {
        string serverId = server.key();
        auto websocketConfig = server.value();

        if (!websocketConfig.is_object()) {
            continue;
        }

        const string ip = websocketConfig["ip"];
        int port = websocketConfig["port"];
        int sslPort = websocketConfig["sslPort"];
        int count = websocketConfig["threads"];

        logInfo << "start websocket server, port: " << port;
        if (port) {
            HttpServer::instance()->start(ip, port, count, false, true);
        }
        logInfo << "start websockets server, sslPort: " << sslPort;
        if (sslPort) {
            HttpServer::instance()->start(ip, sslPort, count, true, true);
        }
    }

    // http server放到最后，等其他协议的api加到httpApi中的mapApi里，避免多线程使用mapApi
    auto httpApiConfigVec = configJson["Http"]["Api"];
    for (auto server : httpApiConfigVec.items()) {
        string serverId = server.key();
        auto httpApiConfig = server.value();

        if (!httpApiConfig.is_object()) {
            continue;
        }

        const string ip = httpApiConfig["ip"];
        int port = httpApiConfig["port"];
        int sslPort = httpApiConfig["sslPort"];
        int count = httpApiConfig["threads"];

        logInfo << "start http api, port: " << port;
        if (port) {
            HttpServer::instance()->start(ip, port, count);
        }
        logInfo << "start https api, sslPort: " << sslPort;
        if (sslPort) {
            HttpServer::instance()->start(ip, sslPort, count, true);
        }
    }

    auto httpServerConfigVec = configJson["Http"]["Server"];
    for (auto server : httpServerConfigVec.items()) {
        string serverId = server.key();
        auto httpServerConfig = server.value();

        if (!httpServerConfig.is_object()) {
            continue;
        }

        const string ip = httpServerConfig["ip"];
        int port = httpServerConfig["port"];
        int sslPort = httpServerConfig["sslPort"];
        int count = httpServerConfig["threads"];

        logInfo << "start http server, port: " << port;
        if (port) {
            HttpServer::instance()->start(ip, port, count);
        }
        logInfo << "start https server, sslPort: " << sslPort;
        if (sslPort) {
            HttpServer::instance()->start(ip, sslPort, count, true);
        }
    }
#endif

#ifdef ENABLE_WEBRTC
    WebrtcClient::init();
    auto WebrtcConfigVec = configJson["Webrtc"]["Server"];
    for (auto server : WebrtcConfigVec.items()) {
        string serverId = server.key();
        auto WebrtcConfig = server.value();

        if (!WebrtcConfig.is_object()) {
            continue;
        }

        const string ip = WebrtcConfig["ip"];
        int port = WebrtcConfig["port"];
        int count = WebrtcConfig["threads"];
        int sockType = WebrtcConfig["sockType"];

        logInfo << "start webrtc server, port: " << port;
        if (port) {
            WebrtcServer::instance()->start(ip, port, count, sockType);
            // RtcServer::Instance().Start(EventLoopPool::instance()->getLoopByCircle(), port, "0.0.0.0");
        }
        // logInfo << "start rtsps server, sslPort: " << sslPort;
        // if (sslPort) {
        //     RtspServer::instance()->start(ip, sslPort, count);
        // }
    }
#endif

#ifdef ENABLE_SRT
    SrtSocket::initSrt();
    auto srtConfigVec = configJson["Srt"]["Server"];
    for (auto server : srtConfigVec.items()) {
        string serverId = server.key();
        auto srtConfig = server.value();

        if (!srtConfig.is_object()) {
            continue;
        }

        const string ip = srtConfig["ip"];
        int port = srtConfig["port"];
        int count = srtConfig["threads"];

        logInfo << "start srt server, port: " << port;
        if (port) {
            SrtServer::instance()->start(ip, port, count, 0);
            // RtcServer::Instance().Start(EventLoopPool::instance()->getLoopByCircle(), port, "0.0.0.0");
        }
        // logInfo << "start rtsps server, sslPort: " << sslPort;
        // if (sslPort) {
        //     RtspServer::instance()->start(ip, sslPort, count);
        // }
    }
#endif

#ifdef ENABLE_SRTCUSTOM
    auto srtCustomConfigVec = configJson["SrtCustom"]["Server"];
    for (auto server : srtCustomConfigVec.items()) {
        string serverId = server.key();
        auto srtCustomConfig = server.value();

        if (!srtCustomConfig.is_object()) {
            continue;
        }

        const string ip = srtCustomConfig["ip"];
        int port = srtCustomConfig["port"];
        int count = srtCustomConfig["threads"];

        logInfo << "start srt custom server, port: " << port;
        if (port) {
            SrtCustomServer::instance()->start(ip, port, count, 3);
            // RtcServer::Instance().Start(EventLoopPool::instance()->getLoopByCircle(), port, "0.0.0.0");
        }
    }
#endif

#if defined(ENABLE_JT808)
    auto jt808ConfigVec = configJson["JT808"]["Server"];
    for (auto server : jt808ConfigVec.items()) {
        string serverId = server.key();
        auto jt808Config = server.value();

        if (!jt808Config.is_object()) {
            continue;
        }

        const string ip = jt808Config["ip"];
        int port = jt808Config["port"];
        int count = jt808Config["threads"];

        logInfo << "start jt808 server, port: " << port;
        if (port) {
            JT808Server::instance()->start(ip, port, count);
        }
    }
#endif

    // auto loop = EventLoopPool::instance()->getLoopByCircle();
    // TcpClient::Ptr client = make_shared<TcpClient>(loop);
    // loop->async([client]() {
    //     client->create("");
    //     client->connect("127.0.0.1", 9000);
    // }, true);

    while (true) {
        // TODO 可做一些巡检工作
        EventLoopPool::instance()->for_each_loop([](const EventLoop::Ptr &loop){
            int lastWaitDuration, lastRunDuration, curWaitDuration, curRunDuration;
            loop->getLoad(lastWaitDuration, lastRunDuration, curWaitDuration, curRunDuration);

            if (curRunDuration > 1000 || lastRunDuration > 1000) {
                logWarn << "thread: looper-" << loop->getEpollFd()
                        << ", lastWaitDuration: " << lastWaitDuration
                        << ", lastRunDuration: " << lastRunDuration
                        << ", curWaitDuration: " << curWaitDuration
                        << ", curRunDuration: " << curRunDuration;
            }
        });
        sleep(5);
    }
#ifdef ENABLE_SRT
    SrtSocket::uninitSrt();
#endif
    return 0;
}
