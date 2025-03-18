#include "JT1078Api.h"
#include "ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Common/Define.h"
#include "Util/String.h"
#include "JT1078/JT1078Connection.h"
#include "JT1078/JT1078Server.h"
#include "JT1078/JT1078Client.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void JT1078Api::initApi()
{
    g_mapApi.emplace("/api/v1/jt1078/create", JT1078Api::create);
    g_mapApi.emplace("/api/v1/jt1078/server/open", JT1078Api::openServer);
    g_mapApi.emplace("/api/v1/jt1078/server/close", JT1078Api::closeServer);
    g_mapApi.emplace("/api/v1/jt1078/talk/start", JT1078Api::startTalk);
    g_mapApi.emplace("/api/v1/jt1078/talk/stop", JT1078Api::stopTalk);
    g_mapApi.emplace("/api/v1/jt1078/send/start", JT1078Api::startSend);
    g_mapApi.emplace("/api/v1/jt1078/send/stop", JT1078Api::stopSend);
    g_mapApi.emplace("/api/v1/jt1078/port/info", JT1078Api::getPortInfo);
}

void JT1078Api::create(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"simCode", "channel", "port", "appName", "streamName"});
    string key = parser._body["simCode"].get<string>() + "_" + parser._body["channel"].get<string>() + "_" + parser._body["port"].get<string>();
    JT1078Info info;
    info.appName = parser._body["appName"];
    info.streamName = parser._body["streamName"];
    bool res = JT1078Connection::addJt1078Info(key, info);

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    if (res) {
        static int createKeyTimeout = Config::instance()->getAndListen([](const json& config){
            createKeyTimeout = Config::instance()->get("JT1078", "Server", "Server1", "createKeyTimeout");
            logInfo << "createKeyTimeout: " << createKeyTimeout;
        }, "JT1078", "Server", "Server1", "createKeyTimeout");

        if (createKeyTimeout == 0) {
            createKeyTimeout = 15;
        }
        
        int expire = createKeyTimeout; //second
        if (parser._body.find("expire") != parser._body.end()) {
            expire = toInt(parser._body["expire"]);
        }

        auto loop = EventLoop::getCurrentLoop();
        loop->addTimerTask(expire * 1000, [key](){
            JT1078Connection::delJt1078Info(key);
            return 0;
        }, nullptr);
        value["code"] = "200";
        value["msg"] = "success";
    } else {
        value["code"] = "400";
        value["msg"] = "add jt1078 info failed";
    }
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void JT1078Api::openServer(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    // checkArgs(parser._body, {"port"});
    int port = JT1078Server::instance()->getPort();
    if (port < 0) {
        value["code"] = "400";
        value["msg"] = "port is empty";
        rsp.setContent(value.dump());
        rspFunc(rsp);
        return ;
    }
    int expire = 15; //second
    if (parser._body.find("expire") != parser._body.end()) {
        expire = toInt(parser._body["expire"]);
    }

    string path;
    string appName;
    if (parser._body.find("appName") != parser._body.end()) {
        if (parser._body.find("streamName") != parser._body.end()) {
            path = "/" + parser._body["appName"].get<string>() + "/" + parser._body["streamName"].get<string>();
        } else {
            appName = parser._body["appName"].get<string>();
        }
    }
    JT1078Server::instance()->setStreamPath(port, path, expire, appName);

    JT1078Server::instance()->start("0.0.0.0", port, 1);

    value["code"] = "200";
    value["msg"] = "success";
    value["port"] = port;
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void JT1078Api::closeServer(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"port"});
    JT1078Server::instance()->stopByPort(toInt(parser._body["port"]), 1);

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void JT1078Api::startTalk(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    // recv参数，对讲设备的参数
    // send参数，准备发往对讲设备的源流的参数
    checkArgs(parser._body, {"recvSimCode", "recvChannel", "recvPort", 
                            "sendAppName", "sendStreamName", "sendSimCode", "sendChannel"});
    string key = parser._body["recvSimCode"].get<string>() + "_" + parser._body["recvChannel"].get<string>()
                 + "_" + parser._body["recvPort"].get<string>();
    JT1078TalkInfo info;
    info.channel = toInt(parser._body["sendChannel"]);
    info.simCode = parser._body["sendSimCode"];
    string appName = parser._body["sendAppName"];
    string streamName = parser._body["sendStreamName"];

    info.urlParser.path_ = "/" + appName + "/" + streamName;
    info.urlParser.vhost_ = DEFAULT_VHOST;
    info.urlParser.protocol_ = PROTOCOL_JT1078;
    info.urlParser.type_ = "talk";
    bool res = JT1078Connection::addTalkInfo(key, info);

    if (res) {
        static int createKeyTimeout = Config::instance()->getAndListen([](const json& config){
            createKeyTimeout = Config::instance()->get("JT1078", "Server", "Server1", "createKeyTimeout");
            logInfo << "createKeyTimeout: " << createKeyTimeout;
        }, "JT1078", "Server", "Server1", "createKeyTimeout");

        if (createKeyTimeout == 0) {
            createKeyTimeout = 15;
        }

        auto loop = EventLoop::getCurrentLoop();
        loop->addTimerTask(createKeyTimeout * 1000, [key](){
            JT1078Connection::delJt1078Info(key);
            return 0;
        }, nullptr);

        value["code"] = "200";
        value["msg"] = "success";
    } else {
        value["code"] = "400";
        value["msg"] = "add jt1078 info failed";
    }
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void JT1078Api::stopTalk(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    UrlParser taslUrlParser;
    checkArgs(parser._body, {"sendAppName", "sendStreamName"});
    string appName = parser._body["sendAppName"];
    string streamName = parser._body["sendStreamName"];
    taslUrlParser.path_ = "/" + appName + "/" + streamName;
    taslUrlParser.vhost_ = DEFAULT_VHOST;
    taslUrlParser.protocol_ = PROTOCOL_JT1078;
    taslUrlParser.type_ = "talk";

    auto source = MediaSource::get(taslUrlParser.path_, taslUrlParser.vhost_, taslUrlParser.protocol_, taslUrlParser.type_);
    if (source) {
        source->release();
    }

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void JT1078Api::startSend(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"url", "appName", "streamName", "simCode", "channel"});

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("JT1078", "Server", "timeout");
    }, "JT1078", "Server", "timeout");

    auto client = make_shared<JT1078Client>(MediaClientType_Push, parser._body["appName"], parser._body["streamName"]);
    client->setSimCode(parser._body["simCode"]);
    client->setChannel(toInt(parser._body["channel"]));
    client->start("0.0.0.0", 0, parser._body["url"], timeout);

    string key = parser._body["url"];
    MediaClient::addMediaClient(key, client);

    client->setOnClose([key](){
        MediaClient::delMediaClient(key);
    });

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void JT1078Api::stopSend(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"url"});

    string key = parser._body["url"];
    MediaClient::delMediaClient(key);

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void JT1078Api::getPortInfo(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto info = JT1078Server::instance()->getPortInfo();
    value["portInfo"]["maxPort"] = info.maxPort_;
    value["portInfo"]["minPort"] = info.minPort_;
    for (auto port : info.portFreeList_) {
        value["portInfo"]["freePort"].emplace_back(port);
    }
    
    for (auto port : info.portUseList_) {
        value["portInfo"]["usePort"].emplace_back(port);
    }

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}
