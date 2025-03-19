#include "RtspApi.h"
#include "Common/ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "Rtsp/RtspClient.h"
#include "Rtsp/RtspServer.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void RtspApi::initApi()
{
    g_mapApi.emplace("/api/v1/rtsp/streams/create", RtspApi::createRtspStream);

    g_mapApi.emplace("/api/v1/rtsp/play/start", RtspApi::startRtspPlay);
    g_mapApi.emplace("/api/v1/rtsp/play/stop", RtspApi::stopRtspPlay);
    g_mapApi.emplace("/api/v1/rtsp/play/list", RtspApi::listRtspPlayInfo);

    g_mapApi.emplace("/api/v1/rtsp/publish/start", RtspApi::startRtspPublish);
    g_mapApi.emplace("/api/v1/rtsp/publish/stop", RtspApi::stopRtspPublish);
    g_mapApi.emplace("/api/v1/rtsp/publish/list", RtspApi::listRtspPublishInfo);

    g_mapApi.emplace("/api/v1/rtsp/server/create", RtspApi::createRtspServer);
    g_mapApi.emplace("/api/v1/rtsp/server/stop", RtspApi::stopRtspServer);
}

void RtspApi::createRtspStream(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    
}

void RtspApi::startRtspPlay(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"url", "appName", "streamName"});

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Rtsp", "Server", "Server1", "timeout");
    }, "Rtsp", "Server", "Server1", "timeout");

    auto rtpType = Transport_TCP;
    if (parser._body.find("rtpType") != parser._body.end()) {
        if (parser._body["rtpType"].get<string>() == "udp") {
            rtpType = Transport_UDP;
        }
    }
    
    auto client = make_shared<RtspClient>(MediaClientType_Pull, parser._body["appName"], parser._body["streamName"]);
    client->setTransType(rtpType);

    client->start("0.0.0.0", 0, parser._body["url"], timeout);
    if (parser._body.find("username") != parser._body.end()) {
        client->setUsername(parser._body["username"]);
    }
    if (parser._body.find("password") != parser._body.end()) {
        string password = parser._body["password"];
        char c = '\\';
        password.erase(std::remove_if(password.begin(), password.end(), [c](unsigned char ch) { 
            return ch == c; 
        }), password.end());
        client->setPassword(password);
    }
    // stringstream key;
    string key = "/" + parser._body["appName"].get<string>() + "/" + parser._body["streamName"].get<string>();
    MediaClient::addMediaClient(key, client);

    client->setOnClose([key](){
        MediaClient::delMediaClient(key);
    });

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RtspApi::stopRtspPlay(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"appName", "streamName"});

    string key = "/" + parser._body["appName"].get<string>() + "/" + parser._body["streamName"].get<string>();
    MediaClient::delMediaClient(key);

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RtspApi::listRtspPlayInfo(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    int count = 0;
    auto allClients = MediaClient::getAllMediaClient();
    for (auto& pr : allClients) {
        string protocol;
        MediaClientType type;
        pr.second->getProtocolAndType(protocol, type);

        if (protocol == "rtsp" && type == MediaClientType_Pull) {
            auto rtspClient = dynamic_pointer_cast<RtspClient>(pr.second);
            if (!rtspClient) {
                continue ;
            }
            json item;
            item["path"] = rtspClient->getPath();
            item["url"] = rtspClient->getSourceUrl();

            value["clients"].push_back(item);
            ++count;
        }
    }

    value["code"] = "200";
    value["msg"] = "success";
    value["count"] = count;
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RtspApi::startRtspPublish(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"url", "appName", "streamName"});

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Rtsp", "Server", "Server1", "timeout");
    }, "Rtsp", "Server", "Server1", "timeout");

    auto rtpType = Transport_TCP;
    if (parser._body.find("rtpType") != parser._body.end()) {
        if (parser._body["rtpType"].get<string>() == "udp") {
            rtpType = Transport_UDP;
        }
    }

    auto client = make_shared<RtspClient>(MediaClientType_Push, parser._body["appName"], parser._body["streamName"]);
    client->setTransType(rtpType);
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

void RtspApi::stopRtspPublish(const HttpParser& parser, const UrlParser& urlParser, 
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

void RtspApi::listRtspPublishInfo(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto allClients = MediaClient::getAllMediaClient();
    for (auto& pr : allClients) {
        string protocol;
        MediaClientType type;
        pr.second->getProtocolAndType(protocol, type);

        if (protocol == "rtsp" && type == MediaClientType_Push) {
            auto rtspClient = dynamic_pointer_cast<RtspClient>(pr.second);
            if (!rtspClient) {
                continue ;
            }
            json item;
            item["path"] = rtspClient->getPath();
            item["url"] = rtspClient->getSourceUrl();

            value["clients"].push_back(item);
        }
    }

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RtspApi::createRtspServer(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"port"});
    string ip = parser._body.value("ip", "0.0.0.0");
    int port = toInt(parser._body["port"]);
    int count = toInt(parser._body.value("count", "0"));

    RtspServer::instance()->start(ip, port, count);

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RtspApi::stopRtspServer(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"port"});
    int port = toInt(parser._body["port"]);
    int count = toInt(parser._body.value("count", "0"));

    RtspServer::instance()->stopByPort(port, count);

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

