#include "UserApi.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Common/ApiUtil.h"
#include "Manager/StreamManager.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void UserApi::initApi()
{
    g_mapApi.emplace("/api/user/login", UserApi::login);
}

void UserApi::login(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "username", "password" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    value["data"]["username"] = "test";
    value["data"]["password"] = "111";
    value["data"]["createTime"] = "1243234";
    value["data"]["updateTime"] = "354353";
    value["data"]["pushKey"] = "key";
    
    value["data"]["role"]["id"] = 432432;
    value["data"]["role"]["name"] = "test";
    value["data"]["role"]["authority"] = "test";
    value["data"]["role"]["createTime"] = "test";
    value["data"]["role"]["updateTime"] = "test";

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setHeader("access-token", "xxxxxxxxxxxxxxx");
    rsp.setContent(value.dump());
    rspFunc(rsp);
}