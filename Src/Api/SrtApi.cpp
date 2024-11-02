#include "SrtApi.h"
#include "Logger.h"
#include "Util/String.h"
#include "ApiUtil.h"
#include "Srt/SrtClient.h"

#include <unordered_map>

using namespace std;

#ifdef ENABLE_SRT

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void SrtApi::initApi()
{
    g_mapApi.emplace("/api/v1/srt/pull", SrtApi::createPullClient);
    g_mapApi.emplace("/api/v1/srt/push", SrtApi::createPushClient);
}

void SrtApi::createPullClient(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"url", "appName", "streamName"});

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Srt", "Server", "Server1", "timeout");
    }, "Srt", "Server", "Server1", "timeout");

    auto client = make_shared<SrtClient>(MediaClientType_Pull, parser._body["appName"], parser._body["streamName"]);
    client->start("0.0.0.0", 0, parser._body["url"], timeout);

    string key = parser._body["url"];
    MediaClient::addMediaClient(key, client);

    client->setOnClose([key](){
        MediaClient::delMediaClient(key);
    });


    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = 200;
    rsp.setHeader("Access-Control-Allow-Origin", "*");
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void SrtApi::createPushClient(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"url", "appName", "streamName"});

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Srt", "Server", "Server1", "timeout");
    }, "Srt", "Server", "Server1", "timeout");

    auto client = make_shared<SrtClient>(MediaClientType_Push, parser._body["appName"], parser._body["streamName"]);
    client->start("0.0.0.0", 0, parser._body["url"], timeout);

    string key = parser._body["url"];
    MediaClient::addMediaClient(key, client);

    client->setOnClose([key](){
        MediaClient::delMediaClient(key);
    });


    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = 200;
    rsp.setHeader("Access-Control-Allow-Origin", "*");
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

#endif