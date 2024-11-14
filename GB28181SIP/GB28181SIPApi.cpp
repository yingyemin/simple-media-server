#include "Api/ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "GB28181SIPApi.h"
#include "GB28181SIP/GB28181SIPManager.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void GB28181SIPApi::initApi()
{
    g_mapApi.emplace("/api/v1/catalog", GB28181SIPApi::catalog);
    g_mapApi.emplace("/api/v1/invite", GB28181SIPApi::invite);
}

void GB28181SIPApi::catalog(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"deviceId"});
    auto deviceCtx = GB28181SIPManager::instance()->getContext(parser._body["deviceId"]);
    deviceCtx->catalog();

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void GB28181SIPApi::invite(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"deviceId", "channelId", "channelNum", "ip", "port", "ssrc"});
    MediaInfo info;
    info.channelId = parser._body["channelId"];
    info.channelNum = toInt(parser._body["channelNum"]);
    info.ip = parser._body["ip"];
    info.port = toInt(parser._body["port"]);
    info.ssrc = toInt(parser._body["ssrc"]);

    auto deviceCtx = GB28181SIPManager::instance()->getContext(parser._body["deviceId"]);
    deviceCtx->invite(info);

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}
