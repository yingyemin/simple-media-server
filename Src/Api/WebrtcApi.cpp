#include "WebrtcApi.h"
#include "Logger.h"
#include "Util/String.h"
#include "ApiUtil.h"
// #include "Webrtc/RtcServer.h"
#include "Webrtc/WebrtcContextManager.h"
#include "Webrtc/WebrtcContext.h"
#include "Webrtc/WebrtcClient.h"

#include <unordered_map>

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void WebrtcApi::initApi()
{
    // server
    g_mapApi.emplace("/api/v1/rtc/play", WebrtcApi::rtcPlay);
    g_mapApi.emplace("/api/v1/rtc/publish", WebrtcApi::rtcPublish);

    g_mapApi.emplace("/api/v1/rtc/whep", WebrtcApi::rtcPlay);
    g_mapApi.emplace("/api/v1/rtc/whip", WebrtcApi::rtcPublish);
    // client
    g_mapApi.emplace("/api/v1/rtc/pull/start", WebrtcApi::startRtcPull);
    g_mapApi.emplace("/api/v1/rtc/pull/stop", WebrtcApi::stopRtcPull);
    g_mapApi.emplace("/api/v1/rtc/pull/list", WebrtcApi::listRtcPull);

    g_mapApi.emplace("/api/v1/rtc/push/start", WebrtcApi::startRtcPush);
    g_mapApi.emplace("/api/v1/rtc/push/stop", WebrtcApi::stopRtcPush);
    g_mapApi.emplace("/api/v1/rtc/push/list", WebrtcApi::listRtcPush);
}

void WebrtcApi::rtcPlay(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    bool isWhep = false;
    if (urlParser.path_.find("whep") != string::npos) {
        isWhep = true;
    }

    string appName;
    string streamName;
    string sdp;
    int enableDtls = 0;
    if (isWhep) {
        enableDtls = true;
        appName = urlParser.vecParam_.at("appName");
        streamName = urlParser.vecParam_.at("streamName");
        sdp = parser._content;
    } else {
        checkArgs(parser._body, {"appName", "streamName", "enableDtls", "sdp"});
        enableDtls = toInt(parser._body["enableDtls"]);
        appName = parser._body["appName"];
        streamName = parser._body["streamName"];
        sdp = parser._body["sdp"];
    }

    auto context = make_shared<WebrtcContext>();
    context->setDtls(enableDtls);
    context->initPlayer(appName, streamName, sdp);

    WebrtcContextManager::instance()->addContext(context->getUsername(), context);

    if (isWhep) {
        HttpResponse rsp;
        rsp._status = 201;
        rsp.setHeader("Access-Control-Allow-Origin", "*");
        rsp.setHeader("Content-Type", "application/sdp");
        string stopUrl = urlParser.protocol_ + "://" + urlParser.host_ + ":" + to_string(urlParser.port_) + "api/v1/rtc/play/stop";
        rsp.setHeader("Location", stopUrl);
        rsp.setContent(context->getLocalSdp());
        rspFunc(rsp);
    } else {
        HttpResponse rsp;
        rsp._status = 200;
        json value;
        value["sdp"] = context->getLocalSdp();
        value["code"] = 200;
        rsp.setHeader("Access-Control-Allow-Origin", "*");
        rsp.setContent(value.dump());
        rspFunc(rsp);
    }
}

void WebrtcApi::rtcPublish(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    bool isWhip = false;
    if (urlParser.path_.find("whip") != string::npos) {
        isWhip = true;
    }

    string appName;
    string streamName;
    string sdp;
    int enableDtls = 0;
    if (isWhip) {
        enableDtls = true;
        appName = urlParser.vecParam_.at("appName");
        streamName = urlParser.vecParam_.at("streamName");
        sdp = parser._content;
    } else {
        checkArgs(parser._body, {"appName", "streamName", "sdp"});

        appName = parser._body["appName"];
        streamName = parser._body["streamName"];
        enableDtls = toInt(parser._body.value("enableDtls", "0"));
        sdp = parser._body["sdp"];
    }

    auto context = make_shared<WebrtcContext>();
    context->setDtls(1);
    context->initPublisher(appName, streamName, sdp);

    WebrtcContextManager::instance()->addContext(context->getUsername(), context);

    if (isWhip) {
        HttpResponse rsp;
        rsp._status = 201;
        rsp.setHeader("Access-Control-Allow-Origin", "*");
        rsp.setHeader("Content-Type", "application/sdp");
        string stopUrl = urlParser.protocol_ + "://" + urlParser.host_ + ":" + to_string(urlParser.port_) + "api/v1/rtc/publish/stop";
        rsp.setHeader("Location", stopUrl);
        rsp.setContent(context->getLocalSdp());
        rspFunc(rsp);
    } else {
        HttpResponse rsp;
        rsp._status = 200;
        json value;
        value["sdp"] = context->getLocalSdp();
        value["code"] = 0;
        value["server"] = "sms";
        value["sessionid"] = "sms";
        rsp.setHeader("Access-Control-Allow-Origin", "*");
        rsp.setContent(value.dump());
        rspFunc(rsp);
    }
}

void WebrtcApi::startRtcPull(const HttpParser& parser, const UrlParser& urlParser, 
             const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"url", "appName", "streamName"});

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Webrtc", "Server", "Server1", "timeout");
    }, "Webrtc", "Server", "Server1", "timeout");
    
    auto client = make_shared<WebrtcClient>(MediaClientType_Pull, parser._body["appName"], parser._body["streamName"]);
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

void WebrtcApi::stopRtcPull(const HttpParser& parser, const UrlParser& urlParser, 
             const function<void(HttpResponse& rsp)>& rspFunc)
{

}

void WebrtcApi::listRtcPull(const HttpParser& parser, const UrlParser& urlParser, 
             const function<void(HttpResponse& rsp)>& rspFunc)
{

}

void WebrtcApi::startRtcPush(const HttpParser& parser, const UrlParser& urlParser, 
             const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"url", "appName", "streamName"});

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Webrtc", "Server", "Server1", "timeout");
    }, "Webrtc", "Server", "Server1", "timeout");

    auto client = make_shared<WebrtcClient>(MediaClientType_Push, parser._body["appName"], parser._body["streamName"]);
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

void WebrtcApi::stopRtcPush(const HttpParser& parser, const UrlParser& urlParser, 
             const function<void(HttpResponse& rsp)>& rspFunc)
{

}

void WebrtcApi::listRtcPush(const HttpParser& parser, const UrlParser& urlParser, 
             const function<void(HttpResponse& rsp)>& rspFunc)
{

}