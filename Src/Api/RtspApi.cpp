#include "RtspApi.h"
#include "ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "Rtsp/RtspClient.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void RtspApi::initApi()
{
    g_mapApi.emplace("/api/v1/rtsp/create", RtspApi::createRtspStream);
    g_mapApi.emplace("/api/v1/rtsp/play/start", RtspApi::startRtspPlay);
    g_mapApi.emplace("/api/v1/rtsp/play/stop", RtspApi::stopRtspPlay);
    g_mapApi.emplace("/api/v1/rtsp/play/list", RtspApi::listRtspPlayInfo);

    g_mapApi.emplace("/api/v1/rtsp/publish/start", RtspApi::startRtspPublish);
    g_mapApi.emplace("/api/v1/rtsp/publish/stop", RtspApi::stopRtspPublish);
    g_mapApi.emplace("/api/v1/rtsp/publish/list", RtspApi::listRtspPublishInfo);
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
    
}

void RtspApi::listRtspPlayInfo(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    
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
    
}

void RtspApi::listRtspPublishInfo(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    
}

