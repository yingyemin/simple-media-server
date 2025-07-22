#ifdef ENABLE_EHOME2

#include "Ehome2Api.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Common/ApiUtil.h"
#include "Util/String.h"
#include "Ehome2/Ehome2VodServer.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void Ehome2Api::initApi()
{
    g_mapApi.emplace("/api/v1/ehome2/vod/create", Ehome2Api::createVodServer);
    g_mapApi.emplace("/api/v1/ehome2/vod/stop", Ehome2Api::stopVodServer);
}

void Ehome2Api::createVodServer(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"ip", "port", "appName", "streamName"});
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    string ip = parser._body["ip"];
    int port = toInt(parser._body["port"]);
    string appName = parser._body["appName"];
    string streamName = parser._body["streamName"];
    string path = "/" + appName + "/" + streamName;

    int expire = toInt(parser._body.value("expire", "15000"));

    int timeout = toInt(parser._body.value("timeout", "5000"));

    Ehome2VodServer::instance()->setServerInfo(port, path, expire, appName, timeout);
    Ehome2VodServer::instance()->start(ip, path, port, 1);

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void Ehome2Api::stopVodServer(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"port"});
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    int port = toInt(parser._body["port"]);
    Ehome2VodServer::instance()->stopByPort(port, 1);

    value["code"] = "200";
    value["msg"] = "success";
    value["authResult"] = true;
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

#endif