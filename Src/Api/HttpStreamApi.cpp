#include "HttpStreamApi.h"
#include "ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "HttpStream/HttpFlvClient.h"
#include "HttpStream/HlsClientContext.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void HttpStreamApi::initApi()
{
    g_mapApi.emplace("/api/v1/http/flv/play/start", HttpStreamApi::startFlvPlay);
    g_mapApi.emplace("/api/v1/http/flv/play/stop", HttpStreamApi::stopFlvPlay);
    g_mapApi.emplace("/api/v1/http/flv/play/list", HttpStreamApi::listFlvPlayInfo);
    
    g_mapApi.emplace("/api/v1/http/hls/play/start", HttpStreamApi::startHlsPlay);
    g_mapApi.emplace("/api/v1/http/hls/play/stop", HttpStreamApi::stopHlsPlay);
    g_mapApi.emplace("/api/v1/http/hls/play/list", HttpStreamApi::listHlsPlayInfo);
}

void HttpStreamApi::startFlvPlay(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"url", "appName", "streamName"});

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Http", "Stream", "Server1", "timeout");
    }, "Http", "Stream", "Server1", "timeout");

    if (timeout == 0) {
        timeout = 5;
    }
    
    auto client = make_shared<HttpFlvClient>(MediaClientType_Pull, parser._body["appName"], parser._body["streamName"]);
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

void HttpStreamApi::stopFlvPlay(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    
}

void HttpStreamApi::listFlvPlayInfo(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    
}

void HttpStreamApi::startHlsPlay(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"url", "appName", "streamName"});

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Http", "Stream", "Server1", "timeout");
    }, "Http", "Stream", "Server1", "timeout");

    if (timeout == 0) {
        timeout = 5;
    }
    
    auto client = make_shared<HlsClientContext>(MediaClientType_Pull, parser._body["appName"], parser._body["streamName"]);
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

void HttpStreamApi::stopHlsPlay(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    
}

void HttpStreamApi::listHlsPlayInfo(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    
}
