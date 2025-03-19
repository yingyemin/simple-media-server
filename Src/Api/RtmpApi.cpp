#include "RtmpApi.h"
#include "Common/ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "Rtmp/RtmpClient.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void RtmpApi::initApi()
{
    g_mapApi.emplace("/api/v1/rtmp/create", RtmpApi::createRtmpStream);
    g_mapApi.emplace("/api/v1/rtmp/play/start", RtmpApi::startRtmpPlay);
    g_mapApi.emplace("/api/v1/rtmp/play/stop", RtmpApi::stopRtmpPlay);
    g_mapApi.emplace("/api/v1/rtmp/play/list", RtmpApi::listRtmpPlayInfo);

    g_mapApi.emplace("/api/v1/rtmp/publish/start", RtmpApi::startRtmpPublish);
    g_mapApi.emplace("/api/v1/rtmp/publish/stop", RtmpApi::stopRtmpPublish);
    g_mapApi.emplace("/api/v1/rtmp/publish/list", RtmpApi::listRtmpPublishInfo);
}

void RtmpApi::createRtmpStream(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    
}

void RtmpApi::startRtmpPlay(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"url", "appName", "streamName"});

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Rtmp", "Server", "Server1", "timeout");
    }, "Rtmp", "Server", "Server1", "timeout");
    
    auto client = make_shared<RtmpClient>(MediaClientType_Pull, parser._body["appName"], parser._body["streamName"]);
    client->start("0.0.0.0", 0, parser._body["url"], timeout);

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

void RtmpApi::stopRtmpPlay(const HttpParser& parser, const UrlParser& urlParser, 
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

void RtmpApi::listRtmpPlayInfo(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    
}

void RtmpApi::startRtmpPublish(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"url", "appName", "streamName"});

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Rtmp", "Server", "Server1", "timeout");
    }, "Rtmp", "Server", "Server1", "timeout");
    
    auto client = make_shared<RtmpClient>(MediaClientType_Push, parser._body["appName"], parser._body["streamName"]);
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

void RtmpApi::stopRtmpPublish(const HttpParser& parser, const UrlParser& urlParser, 
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

void RtmpApi::listRtmpPublishInfo(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    
}

