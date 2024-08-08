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
    // client
    g_mapApi.emplace("/api/v1/rtc/pull/start", WebrtcApi::startRtcPull);
    g_mapApi.emplace("/api/v1/rtc/pull/stop", WebrtcApi::stopRtcPull);
    g_mapApi.emplace("/api/v1/rtc/pull/list", WebrtcApi::listRtcPull);

    g_mapApi.emplace("/api/v1/rtc/push/start", WebrtcApi::startRtcPush);
    g_mapApi.emplace("/api/v1/rtc/push/start", WebrtcApi::stopRtcPush);
    g_mapApi.emplace("/api/v1/rtc/push/start", WebrtcApi::listRtcPush);
}

void WebrtcApi::rtcPlay(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"appName", "streamName", "enableDtls", "sdp"});

    auto context = make_shared<WebrtcContext>();
    context->setDtls(toInt(parser._body["enableDtls"]));
    context->initPlayer(parser._body["appName"], parser._body["streamName"], parser._body["sdp"]);

    WebrtcContextManager::instance()->addContext(context->getUsername(), context);

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["sdp"] = context->getLocalSdp();
    value["code"] = 200;
    rsp.setHeader("Access-Control-Allow-Origin", "*");
    rsp.setContent(value.dump());
    rspFunc(rsp);


    // checkArgs(parser._body, {"streamurl", "sdp"});
    // string url = parser._body["streamurl"];
    // string sdp = parser._body["sdp"];

    // std::ostringstream oss;
    // std::string session_id;

    // HttpResponse rsp;
    // rsp._status = 200;
    // json value;
    // value["code"] = 200;
    // rsp.setHeader("Access-Control-Allow-Origin", "*");

    // if (0 == RtcServer::Instance().OnReceivedSdp(url, sdp, oss, session_id)) {
    //     //val["code"] = 0;
    //     value["server"] = "14091";
    //     value["sdp"] = oss.str();
    //     value["sessionid"] = session_id;
    //     logInfo << "sdp exchange successed, session is [" << session_id << "]";
    // } else {
    //     value["code"] = 20001;
    // }

    
    // rsp.setContent(value.dump());
    // rspFunc(rsp);
}

void WebrtcApi::rtcPublish(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"streamurl", "sdp"});
    UrlParser streamurlparser;
    streamurlparser.parse(parser._body["streamurl"]);

    auto pos = streamurlparser.path_.find_first_of("/", 1);
    if (pos == string::npos) {
        HttpResponse rsp;
        rsp._status = 200;
        json value;
        value["msg"] = "url is invalid";
        value["code"] = 200;
        rsp.setHeader("Access-Control-Allow-Origin", "*");
        rsp.setContent(value.dump());
        rspFunc(rsp);

        return ;
    }

    auto appName = streamurlparser.path_.substr(1, pos - 1);
    auto streamName = streamurlparser.path_.substr(pos + 1);

    auto context = make_shared<WebrtcContext>();
    context->setDtls(1);
    context->initPublisher(appName, streamName, parser._body["sdp"]);

    WebrtcContextManager::instance()->addContext(context->getUsername(), context);

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