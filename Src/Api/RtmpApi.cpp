#ifdef ENABLE_RTMP

#include "RtmpApi.h"
#include "Common/ApiUtil.h"
#include "Common/ApiResponse.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "Rtmp/RtmpClient.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void RtmpApi::initApi()
{
    g_mapApi.emplace("/api/v1/rtmp/create", RtmpApi::createRtmpStream);
    g_mapApi.emplace("/api/v1/rtmp/play/start", RtmpApi::startRtmpPlay);
    g_mapApi.emplace("/api/v1/rtmp/play/stop", RtmpApi::stopRtmpPlay);
    g_mapApi.emplace("/api/v1/rtmp/play/list", RtmpApi::listRtmpPlayInfo);

    g_mapApi.emplace("/api/v1/rtmp/publish/start", RtmpApi::startRtmpPublish);
    g_mapApi.emplace("/api/v1/rtmp/publish/stop", RtmpApi::stopRtmpPublish);
    g_mapApi.emplace("/api/v1/rtmp/publish/list", RtmpApi::listRtmpPublishInfo);
}

void RtmpApi::createRtmpStream(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    try {
        // 定义参数验证规则
        vector<ParamRule> rules;

        ParamRule appNameRule;
        appNameRule.name = "appName";
        appNameRule.required = true;
        appNameRule.type = "string";
        appNameRule.minLength = 1;
        appNameRule.maxLength = 50;
        appNameRule.pattern = "^[a-zA-Z0-9_-]+$";
        rules.push_back(appNameRule);

        ParamRule streamNameRule;
        streamNameRule.name = "streamName";
        streamNameRule.required = true;
        streamNameRule.type = "string";
        streamNameRule.minLength = 1;
        streamNameRule.maxLength = 100;
        streamNameRule.pattern = "^[a-zA-Z0-9_-]+$";
        rules.push_back(streamNameRule);

        ParamRule streamTypeRule;
        streamTypeRule.name = "streamType";
        streamTypeRule.required = false;
        streamTypeRule.type = "string";
        streamTypeRule.allowedValues = {"live", "record", "playback"};
        rules.push_back(streamTypeRule);

        ParamRule codecRule;
        codecRule.name = "codec";
        codecRule.required = false;
        codecRule.type = "string";
        codecRule.allowedValues = {"h264", "h265", "vp8", "vp9"};
        rules.push_back(codecRule);

        ParamRule resolutionRule;
        resolutionRule.name = "resolution";
        resolutionRule.required = false;
        resolutionRule.type = "string";
        resolutionRule.pattern = "^\\d+x\\d+$";
        rules.push_back(resolutionRule);

        ParamRule bitrateRule;
        bitrateRule.name = "bitrate";
        bitrateRule.required = false;
        bitrateRule.type = "int";
        bitrateRule.minValue = 100;
        bitrateRule.maxValue = 50000;
        rules.push_back(bitrateRule);

        ParamRule framerateRule;
        framerateRule.name = "framerate";
        framerateRule.required = false;
        framerateRule.type = "int";
        framerateRule.minValue = 1;
        framerateRule.maxValue = 120;
        rules.push_back(framerateRule);

        // 验证参数
        validateParams(parser._body, rules);

        // 获取参数值
        string appName = parser._body["appName"];
        string streamName = parser._body["streamName"];
        string streamType = parser._body.value("streamType", "live");
        string codec = parser._body.value("codec", "h264");
        string resolution = parser._body.value("resolution", "1280x720");
        int bitrate = toInt(parser._body.value("bitrate", "2000"));
        int framerate = toInt(parser._body.value("framerate", "25"));
        string description = parser._body.value("description", "");

        // 生成流路径
        string streamPath = "rtmp://localhost:1935/" + appName + "/" + streamName;

        // 构建响应数据
        json responseData;
        responseData["streamPath"] = streamPath;
        responseData["appName"] = appName;
        responseData["streamName"] = streamName;
        responseData["streamType"] = streamType;
        responseData["codec"] = codec;
        responseData["resolution"] = resolution;
        responseData["bitrate"] = bitrate;
        responseData["framerate"] = framerate;
        responseData["description"] = description;

        ApiResponse::success(rspFunc, responseData, "RTMP流创建成功");

    } catch (const ApiException& ex) {
        ApiResponse::error(rspFunc, ex);
    } catch (const exception& ex) {
        ApiResponse::error(rspFunc, ApiErrorCode::INTERNAL_ERROR, ex.what());
    }
}

void RtmpApi::startRtmpPlay(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"url", "appName", "streamName"});

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Rtmp", "Server", "Server1", "timeout");
    }, "Rtmp", "Server", "Server1", "timeout");

    auto client = make_shared<RtmpClient>(MediaClientType_Pull, parser._body["appName"], parser._body["streamName"]);
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

void RtmpApi::stopRtmpPlay(const HttpParser& parser, const UrlParser& urlParser,
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

void RtmpApi::listRtmpPlayInfo(const HttpParser& parser, const UrlParser& urlParser,
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

        if (protocol == "rtmp" && type == MediaClientType_Pull) {
            auto rtmpClient = dynamic_pointer_cast<RtmpClient>(pr.second);
            if (!rtmpClient) {
                continue ;
            }
            json item;
            item["path"] = rtmpClient->getPath();
            item["url"] = rtmpClient->getSourceUrl();

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

void RtmpApi::startRtmpPublish(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"url", "appName", "streamName"});

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Rtmp", "Server", "Server1", "timeout");
    }, "Rtmp", "Server", "Server1", "timeout");

    auto client = make_shared<RtmpClient>(MediaClientType_Push, parser._body["appName"], parser._body["streamName"]);
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

void RtmpApi::stopRtmpPublish(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"url"});

    string key = parser._body["url"];
    MediaClient::delMediaClient(key);

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RtmpApi::listRtmpPublishInfo(const HttpParser& parser, const UrlParser& urlParser,
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

        if (protocol == "rtmp" && type == MediaClientType_Push) {
            auto rtmpClient = dynamic_pointer_cast<RtmpClient>(pr.second);
            if (!rtmpClient) {
                continue ;
            }
            json item;
            item["path"] = rtmpClient->getPath();
            item["url"] = rtmpClient->getSourceUrl();

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