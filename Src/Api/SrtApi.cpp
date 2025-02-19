#include "SrtApi.h"
#include "Logger.h"
#include "Util/String.h"
#include "ApiUtil.h"
#include "Srt/SrtClient.h"

#include <unordered_map>

using namespace std;

#ifdef ENABLE_SRT

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void SrtApi::initApi()
{
    g_mapApi.emplace("/api/v1/srt/pull/start", SrtApi::createPullClient);
    g_mapApi.emplace("/api/v1/srt/pull/stop", SrtApi::stopPullClient);
    g_mapApi.emplace("/api/v1/srt/pull/list", SrtApi::listPullClient);

    g_mapApi.emplace("/api/v1/srt/push/start", SrtApi::createPushClient);
    g_mapApi.emplace("/api/v1/srt/push/stop", SrtApi::stopPushClient);
    g_mapApi.emplace("/api/v1/srt/push/list", SrtApi::listPushClient);
}

void SrtApi::createPullClient(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"url", "appName", "streamName"});

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Srt", "Server", "Server1", "timeout");
    }, "Srt", "Server", "Server1", "timeout");

    auto client = make_shared<SrtClient>(MediaClientType_Pull, parser._body["appName"], parser._body["streamName"]);
    client->start("0.0.0.0", 0, parser._body["url"], timeout);

    string key = parser._body["url"];
    MediaClient::addMediaClient(key, client);

    client->setOnClose([key](){
        MediaClient::delMediaClient(key);
    });


    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = 200;
    rsp.setHeader("Access-Control-Allow-Origin", "*");
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void SrtApi::stopPullClient(const HttpParser& parser, const UrlParser& urlParser, 
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

void SrtApi::listPullClient(const HttpParser& parser, const UrlParser& urlParser, 
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

        if (protocol == "srt" && type == MediaClientType_Pull) {
            auto srtClient = dynamic_pointer_cast<SrtClient>(pr.second);
            if (!srtClient) {
                continue ;
            }
            json item;
            item["path"] = srtClient->getPath();
            item["url"] = srtClient->getSourceUrl();

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

void SrtApi::createPushClient(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"url", "appName", "streamName"});

    static int timeout = Config::instance()->getAndListen([](const json &config){
        timeout = Config::instance()->get("Srt", "Server", "Server1", "timeout");
    }, "Srt", "Server", "Server1", "timeout");

    auto client = make_shared<SrtClient>(MediaClientType_Push, parser._body["appName"], parser._body["streamName"]);
    client->start("0.0.0.0", 0, parser._body["url"], timeout);

    string key = parser._body["url"];
    MediaClient::addMediaClient(key, client);

    client->setOnClose([key](){
        MediaClient::delMediaClient(key);
    });


    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = 200;
    rsp.setHeader("Access-Control-Allow-Origin", "*");
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void SrtApi::stopPushClient(const HttpParser& parser, const UrlParser& urlParser, 
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

void SrtApi::listPushClient(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto allClients = MediaClient::getAllMediaClient();
    for (auto& pr : allClients) {
        string protocol;
        MediaClientType type;
        pr.second->getProtocolAndType(protocol, type);

        if (protocol == "srt" && type == MediaClientType_Push) {
            auto srtClient = dynamic_pointer_cast<SrtClient>(pr.second);
            if (!srtClient) {
                continue ;
            }
            json item;
            item["path"] = srtClient->getPath();
            item["url"] = srtClient->getSourceUrl();

            value["clients"].push_back(item);
        }
    }

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

#endif