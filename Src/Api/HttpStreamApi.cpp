﻿#include "HttpStreamApi.h"
#include "Common/ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "HttpStream/HttpFlvClient.h"
#include "HttpStream/HlsClientContext.h"
#include "HttpStream/HttpPsVodClient.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void HttpStreamApi::initApi()
{
#ifdef ENABLE_RTMP
    g_mapApi.emplace("/api/v1/http/flv/play/start", HttpStreamApi::startFlvPlay);
    g_mapApi.emplace("/api/v1/http/flv/play/stop", HttpStreamApi::stopFlvPlay);
    g_mapApi.emplace("/api/v1/http/flv/play/list", HttpStreamApi::listFlvPlayInfo);
#endif
    
#ifdef ENABLE_HLS
    g_mapApi.emplace("/api/v1/http/hls/play/start", HttpStreamApi::startHlsPlay);
    g_mapApi.emplace("/api/v1/http/hls/play/stop", HttpStreamApi::stopHlsPlay);
    g_mapApi.emplace("/api/v1/http/hls/play/list", HttpStreamApi::listHlsPlayInfo);
#endif

#ifdef ENABLE_MPEG
    g_mapApi.emplace("/api/v1/http/ps/vod/play/start", HttpStreamApi::startPsVodPlay);
    g_mapApi.emplace("/api/v1/http/ps/vod/play/stop", HttpStreamApi::stopPsVodPlay);
    g_mapApi.emplace("/api/v1/http/ps/vod/play/list", HttpStreamApi::listPsVodPlayInfo);
#endif
}

#ifdef ENABLE_RTMP
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
#endif

#ifdef ENABLE_HLS
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
#endif

#ifdef ENABLE_MPEG
void HttpStreamApi::startPsVodPlay(const HttpParser& parser, const UrlParser& urlParser, 
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
    
    auto client = make_shared<HttpPsVodClient>(MediaClientType_Pull, parser._body["appName"], parser._body["streamName"]);
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

void HttpStreamApi::stopPsVodPlay(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    
}

void HttpStreamApi::listPsVodPlayInfo(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    
}
#endif
