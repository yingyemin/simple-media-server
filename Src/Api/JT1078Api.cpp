#include "JT1078Api.h"
#include "ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "JT1078/JT1078Connection.h"
#include "JT1078/JT1078Server.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void JT1078Api::initApi()
{
    g_mapApi.emplace("/api/v1/jt1078/create", JT1078Api::create);
    g_mapApi.emplace("/api/v1/jt1078/server/open", JT1078Api::openServer);
    g_mapApi.emplace("/api/v1/jt1078/server/close", JT1078Api::closeServer);
}

void JT1078Api::create(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"simCode", "channel", "port", "appName", "streamName"});
    string key = parser._body["simCode"].get<string>() + "_" + parser._body["channel"].get<string>() + "_" + parser._body["port"].get<string>();
    JT1078Info info;
    info.appName = parser._body["appName"];
    info.streamName = parser._body["streamName"];
    JT1078Connection::addJt1078Info(key, info);

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void JT1078Api::openServer(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"port", "appName", "streamName"});
    string path = "/" + parser._body["appName"].get<string>() + "/" + parser._body["streamName"].get<string>();
    JT1078Server::instance()->setStreamPath(toInt(parser._body["port"]), path);

    JT1078Server::instance()->start("0.0.0.0", toInt(parser._body["port"]), 1);

    value["code"] = "200";
    value["msg"] = "success";
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
