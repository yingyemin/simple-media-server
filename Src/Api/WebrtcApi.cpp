#ifdef ENABLE_WEBRTC

#include "WebrtcApi.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/ApiUtil.h"
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
    context->setParams(urlParser.param_);

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
    string preferVideoCodec;
    string preferAudioCodec;
    string sdp;
    int enableDtls = 0;
    if (isWhip) {
        enableDtls = true;
        appName = urlParser.vecParam_.at("appName");
        streamName = urlParser.vecParam_.at("streamName");
        preferVideoCodec = urlParser.vecParam_.at("preferVideoCodec");
        preferAudioCodec = urlParser.vecParam_.at("preferAudioCodec");
        sdp = parser._content;
    } else {
        checkArgs(parser._body, {"appName", "streamName", "sdp"});

        appName = parser._body["appName"];
        streamName = parser._body["streamName"];
        preferVideoCodec = parser._body.value("preferVideoCodec", "");
        preferAudioCodec = parser._body.value("preferAudioCodec", "");
        enableDtls = toInt(parser._body.value("enableDtls", "1"));
        sdp = parser._body["sdp"];
    }

    auto context = make_shared<WebrtcContext>();
    context->setDtls(enableDtls);
    if (!preferVideoCodec.empty()) {
        context->setPreferVideoCodec(preferVideoCodec);
    }
    if (!preferAudioCodec.empty()) {
        context->setPreferAudioCodec(preferAudioCodec);
    }
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

    string apiUrl = UrlParser::urlDecode(parser._body["url"]);

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Webrtc", "Server", "Server1", "timeout");
    }, "Webrtc", "Server", "Server1", "timeout");

    auto client = make_shared<WebrtcClient>(MediaClientType_Pull, parser._body["appName"], parser._body["streamName"]);
    client->start("0.0.0.0", 0, apiUrl, timeout);

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
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"appName", "streamName"});

    string key = "/" + parser._body["appName"].get<string>() + "/" + parser._body["streamName"].get<string>();
    MediaClient::delMediaClient(key);

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void WebrtcApi::listRtcPull(const HttpParser& parser, const UrlParser& urlParser,
             const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    int count = 0;
    auto allClients = MediaClient::getAllMediaClient();
    for (auto& pr : allClients) {
        string protocol;
        MediaClientType type;
        pr.second->getProtocolAndType(protocol, type);

        if (protocol == "webrtc" && type == MediaClientType_Pull) {
            auto webrtcClient = dynamic_pointer_cast<WebrtcClient>(pr.second);
            if (!webrtcClient) {
                continue ;
            }
            json item;
            item["path"] = webrtcClient->getPath();
            item["url"] = webrtcClient->getSourceUrl();

            value["clients"].push_back(item);
            ++count;
        }
    }

    value["code"] = "200";
    value["msg"] = "success";
    value["count"] = count;
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void WebrtcApi::startRtcPush(const HttpParser& parser, const UrlParser& urlParser,
             const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"url", "appName", "streamName"});

    string apiUrl = UrlParser::urlDecode(parser._body["url"]);

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Webrtc", "Server", "Server1", "timeout");
    }, "Webrtc", "Server", "Server1", "timeout");

    auto client = make_shared<WebrtcClient>(MediaClientType_Push, parser._body["appName"], parser._body["streamName"]);
    client->start("0.0.0.0", 0, apiUrl, timeout);

    string key = apiUrl;
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
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"url"});

    string key = UrlParser::urlDecode(parser._body["url"]);
    MediaClient::delMediaClient(key);

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void WebrtcApi::listRtcPush(const HttpParser& parser, const UrlParser& urlParser,
             const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    int count = 0;
    auto allClients = MediaClient::getAllMediaClient();
    for (auto& pr : allClients) {
        string protocol;
        MediaClientType type;
        pr.second->getProtocolAndType(protocol, type);

        if (protocol == "webrtc" && type == MediaClientType_Push) {
            auto webrtcClient = dynamic_pointer_cast<WebrtcClient>(pr.second);
            if (!webrtcClient) {
                continue ;
            }
            json item;
            item["path"] = webrtcClient->getPath();
            item["url"] = webrtcClient->getSourceUrl();

            value["clients"].push_back(item);
            ++count;
        }
    }

    value["code"] = "200";
    value["msg"] = "success";
    value["count"] = count;
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

#endif