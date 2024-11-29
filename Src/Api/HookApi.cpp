#include "HookApi.h"
#include "Logger.h"
#include "Common/Config.h"
#include "ApiUtil.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void HookApi::initApi()
{
    g_mapApi.emplace("/api/v1/onStreamStatus", HookApi::onStreamStatus);
    g_mapApi.emplace("/api/v1/onPublish", HookApi::onPublish);
    g_mapApi.emplace("/api/v1/onPlay", HookApi::onPlay);
    g_mapApi.emplace("/api/v1/onNonePlayer", HookApi::onNonePlayer);
    g_mapApi.emplace("/api/v1/onKeepAlive", HookApi::onKeepAlive);
}

void HookApi::onStreamStatus(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HookApi::onPublish(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    value["authResult"] = true;
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HookApi::onPlay(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    value["authResult"] = true;
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HookApi::onNonePlayer(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    static int stopNonePlayerStream = Config::instance()->getAndListen([](const json& config){
        stopNonePlayerStream = Config::instance()->get("Util", "stopNonePlayerStream");
    }, "Util", "stopNonePlayerStream");

    static int nonePlayerWaitTime = Config::instance()->getAndListen([](const json& config){
        nonePlayerWaitTime = Config::instance()->get("Util", "nonePlayerWaitTime");
    }, "Util", "nonePlayerWaitTime");

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    value["stop"] = stopNonePlayerStream;
    value["delay"] = nonePlayerWaitTime;
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HookApi::onKeepAlive(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}
