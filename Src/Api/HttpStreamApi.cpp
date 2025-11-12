#include "HttpStreamApi.h"
#include "Http/ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#if defined(_WIN32)
#include "Util/String.hpp"
#else
#include "Util/String.hpp"
#endif
#include "Common/MediaClient.h"
#include "HttpStream/HttpFlvClient.h"
#include "HttpStream/HlsClientContext.h"
#include "HttpStream/HttpPsVodClient.h"
#include <ctime>

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
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    try {
        checkArgs(parser._body, {"appName", "streamName"});

        string key = "/" + parser._body["appName"].get<string>() + "/" + parser._body["streamName"].get<string>();

        auto client = MediaClient::getMediaClient(key);
        if (client) {
            client->stop();
            MediaClient::delMediaClient(key);
            value["code"] = "200";
            value["msg"] = "FLV播放停止成功";
        } else {
            value["code"] = "404";
            value["msg"] = "未找到指定的FLV播放流";
        }
    } catch (const exception& e) {
        value["code"] = "400";
        value["msg"] = string("停止FLV播放失败: ") + e.what();
    }

    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HttpStreamApi::listFlvPlayInfo(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    try {
        value["streams"] = json::array();
        int count = 0;

        auto allClients = MediaClient::getAllMediaClient();
        for (auto& pr : allClients) {
            string protocol;
            MediaClientType type;
            pr.second->getProtocolAndType(protocol, type);

            // 检查是否为FLV播放客户端
            if (protocol == "flv" && type == MediaClientType_Pull) {
                json item;
                item["key"] = pr.first;
                item["path"] = pr.first;

                // 解析路径获取appName和streamName
                auto pathParts = split(pr.first, "/");
                if (pathParts.size() >= 2) {
                    item["appName"] = pathParts[0];
                    item["streamName"] = pathParts[1];
                }

                item["status"] = "playing";
                item["createTime"] = time(nullptr);

                value["streams"].push_back(item);
                ++count;
            }
        }

        value["code"] = "200";
        value["msg"] = "success";
        value["count"] = count;
    } catch (const exception& e) {
        value["code"] = "500";
        value["msg"] = string("获取FLV播放列表失败: ") + e.what();
        value["streams"] = json::array();
        value["count"] = 0;
    }

    rsp.setContent(value.dump());
    rspFunc(rsp);
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
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    try {
        checkArgs(parser._body, {"appName", "streamName"});

        string key = "/" + parser._body["appName"].get<string>() + "/" + parser._body["streamName"].get<string>();

        auto client = MediaClient::getMediaClient(key);
        if (client) {
            client->stop();
            MediaClient::delMediaClient(key);
            value["code"] = "200";
            value["msg"] = "HLS播放停止成功";
        } else {
            value["code"] = "404";
            value["msg"] = "未找到指定的HLS播放流";
        }
    } catch (const exception& e) {
        value["code"] = "400";
        value["msg"] = string("停止HLS播放失败: ") + e.what();
    }

    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HttpStreamApi::listHlsPlayInfo(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    try {
        value["streams"] = json::array();
        int count = 0;

        auto allClients = MediaClient::getAllMediaClient();
        for (auto& pr : allClients) {
            string protocol;
            MediaClientType type;
            pr.second->getProtocolAndType(protocol, type);

            // 检查是否为HLS播放客户端
            if (protocol == "hls" && type == MediaClientType_Pull) {
                json item;
                item["key"] = pr.first;
                item["path"] = pr.first;

                // 解析路径获取appName和streamName
                auto pathParts = split(pr.first, "/");
                if (pathParts.size() >= 2) {
                    item["appName"] = pathParts[0];
                    item["streamName"] = pathParts[1];
                }

                item["status"] = "playing";
                item["createTime"] = time(nullptr);

                value["streams"].push_back(item);
                ++count;
            }
        }

        value["code"] = "200";
        value["msg"] = "success";
        value["count"] = count;
    } catch (const exception& e) {
        value["code"] = "500";
        value["msg"] = string("获取HLS播放列表失败: ") + e.what();
        value["streams"] = json::array();
        value["count"] = 0;
    }

    rsp.setContent(value.dump());
    rspFunc(rsp);
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
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    try {
        checkArgs(parser._body, {"appName", "streamName"});

        string key = "/" + parser._body["appName"].get<string>() + "/" + parser._body["streamName"].get<string>();

        auto client = MediaClient::getMediaClient(key);
        if (client) {
            client->stop();
            MediaClient::delMediaClient(key);
            value["code"] = "200";
            value["msg"] = "PS点播停止成功";
        } else {
            value["code"] = "404";
            value["msg"] = "未找到指定的PS点播流";
        }
    } catch (const exception& e) {
        value["code"] = "400";
        value["msg"] = string("停止PS点播失败: ") + e.what();
    }

    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HttpStreamApi::listPsVodPlayInfo(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    try {
        value["streams"] = json::array();
        int count = 0;

        auto allClients = MediaClient::getAllMediaClient();
        for (auto& pr : allClients) {
            string protocol;
            MediaClientType type;
            pr.second->getProtocolAndType(protocol, type);

            // 检查是否为PS点播客户端
            if (protocol == "ps" && type == MediaClientType_Pull) {
                json item;
                item["key"] = pr.first;
                item["path"] = pr.first;

                // 解析路径获取appName和streamName
                auto pathParts = split(pr.first, "/");
                if (pathParts.size() >= 2) {
                    item["appName"] = pathParts[0];
                    item["streamName"] = pathParts[1];
                }

                item["status"] = "playing";
                item["createTime"] = time(nullptr);

                value["streams"].push_back(item);
                ++count;
            }
        }

        value["code"] = "200";
        value["msg"] = "success";
        value["count"] = count;
    } catch (const exception& e) {
        value["code"] = "500";
        value["msg"] = string("获取PS点播列表失败: ") + e.what();
        value["streams"] = json::array();
        value["count"] = 0;
    }

    rsp.setContent(value.dump());
    rspFunc(rsp);
}
#endif
