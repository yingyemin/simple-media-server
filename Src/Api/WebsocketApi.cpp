#include "WebsocketApi.h"
#include "ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "Http/WebsocketClient.h"

using namespace std;

static mutex g_mtxWebsocketClient;
static unordered_map<string, WebsocketClient::Ptr> g_mapWebsocketClient;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void WebsocketApi::initApi()
{
    g_mapApi.emplace("/api/v1/websocket/publish/start", WebsocketApi::startPublish);
}

void WebsocketApi::startPublish(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["msg"] = "success";

    checkArgs(parser._body, {"url", "appName", "streamName"});

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Http", "Stream", "Server1", "timeout");
    }, "Http", "Stream", "Server1", "timeout");

    if (timeout == 0) {
        timeout = 5;
    }
    
    auto client = make_shared<WebsocketClient>(EventLoop::getCurrentLoop());
    client->addHeader("Content-Type", "application/json;charset=UTF-8");
    client->setMethod("GET");
    string uri = "/" + parser._body["appName"].get<string>() + "/" + parser._body["streamName"].get<string>();
    string url = parser._body["url"];
    client->setUri(uri);
    client->setOnClose([url](){
        lock_guard<mutex> lck(g_mtxWebsocketClient);
        g_mapWebsocketClient.erase(url);
    });

    if (client->sendHeader(url, timeout) != 0) {
        value["msg"] = "connect to url: " + url + " failed";
    }

    {
        lock_guard<mutex> lck(g_mtxWebsocketClient);
        auto ret = g_mapWebsocketClient.emplace(url, client);
        if (!ret.second) {
            value["msg"] = "add failed";
        }
    }

    value["code"] = "200";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}
