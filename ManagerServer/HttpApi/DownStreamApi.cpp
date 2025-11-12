#include "DownStreamApi.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Common/ApiUtil.h"
#include "Manager/StreamManager.h"
#include "Util/String.hpp"
#include "Manager/SMSServer.h"
#include "Manager/Device.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void DownStreamApi::initApi()
{
    g_mapApi.emplace("/api/v1/down/stream/onStreamStatus", DownStreamApi::onStreamStatus);
    g_mapApi.emplace("/api/v1/down/stream/onRecordInfo", DownStreamApi::onRecordInfo);
    g_mapApi.emplace("/api/v1/down/stream/push/list", DownStreamApi::listPushStream);
    g_mapApi.emplace("/api/v1/down/stream/push/playUrl", DownStreamApi::getPlayUrlByPush);
    g_mapApi.emplace("/api/v1/down/stream/pull/list", DownStreamApi::listPullStream);
    g_mapApi.emplace("/api/v1/down/stream/list", DownStreamApi::listAllStream);
}

void DownStreamApi::onStreamStatus(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "uri", "status", "serverId" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    std::string uri = parser._body["uri"];
    auto vec = split(uri, "/");
    auto app = vec[0];
    auto streamId = vec[vec.size() - 1];

    auto protocol = parser._body.value("protocol", "rtp");
    auto action = parser._body.value("action", "push");
    if (protocol == "rtp") {
        auto stream = StreamManager::instance()->GetStream(streamId);
        if (stream == nullptr)
        {
            // value["code"] = "400";
            // value["msg"] = "stream not found";
            // rsp.setContent(value.dump());
            // rspFunc(rsp);
            // return;
            auto stream = make_shared<MediaStream>(app, streamId, STREAM_TYPE::STREAM_TYPE_GB);
            stream->setStatus(parser._body["status"]);
            stream->setServerId(parser._body["serverId"]);
            stream->setProtocol(parser._body["protocol"]);
            StreamManager::instance()->AddStream(stream);
        } else {
            stream->setStatus(parser._body["status"]);
            if (parser._body["status"] == "off") {
                StreamManager::instance()->RemoveStream(stream->GetStreamID());
            }
        }
    } else if (action == "push") {
        auto stream = make_shared<MediaStreamPushed>(app, streamId);
        stream->setStatus(parser._body["status"]);
        stream->setServerId(parser._body["serverId"]);
        stream->setProtocol(parser._body["protocol"]);
        if (parser._body["status"] == "on") {
            StreamManager::instance()->AddStream(stream);
        } else {
            StreamManager::instance()->RemoveStream(stream->GetStreamID());
        }
    } else if (action == "pull") {
        auto stream = make_shared<MediaStreamProxy>(app, streamId);
        stream->setStatus(parser._body["status"]);
        stream->setServerId(parser._body["serverId"]);
        stream->setProtocol(parser._body["protocol"]);
        if (parser._body["status"] == "on") {
            StreamManager::instance()->AddStream(stream);
        } else {
            StreamManager::instance()->RemoveStream(stream->GetStreamID());
        }
    }
    

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void DownStreamApi::onRecordInfo(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "app", "stream", "serverId", "status" });
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    CloudRecordInfo::Ptr info = make_shared<CloudRecordInfo>();
    info->app = parser._body["app"];
    info->stream = parser._body["stream"];
    info->mediaServerId = parser._body["serverId"];
    info->startTime = getInt(parser._body, "startTime", 0);
    info->endTime = getInt(parser._body, "endTime", 0);
    info->fileSize = getInt(parser._body, "fileSize", 0);
    info->timeLen = getInt(parser._body, "duration", 0);
    info->fileName = parser._body.value("fileName", "");
    info->filePath = parser._body.value("filePath", "");
    info->fileName = parser._body.value("fileName", "");

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void DownStreamApi::listPushStream(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto list = StreamManager::instance()->GetAllStream();
    for (auto &stream : list) {
        if (stream->GetType() != STREAM_TYPE_PUSH) {
            continue;
        }

        json item;
        item["id"] = 1;
        item["app"] = stream->GetApp();
        item["stream"] = stream->GetStreamID();
        item["totalReaderCount"] = 1;
        item["originType"] = 1;
        item["originTypeStr"] = "protocol-action";
        item["mediaServerId"] = stream->getServerId();
        item["pushIng"] = true;

        value["data"]["list"].push_back(item);
    }
    value["data"]["total"] = list.size();

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void DownStreamApi::getPlayUrlByPush(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "app", "stream", "mediaServerId" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto stream = StreamManager::instance()->GetStream(parser._body["stream"]);
    if (!stream) {
        value["code"] = 404;
        value["msg"] = "stream not found";
        rsp.setContent(value.dump());
        rspFunc(rsp);

        return ;
    }
    auto server = SMSServer::instance()->getServer(parser._body["mediaServerId"]);
    if (!server) {
        value["code"] = 404;
        value["msg"] = "server not found";
        rsp.setContent(value.dump());
        rspFunc(rsp);

        return ;
    }

    auto serverInfo = server->getInfo();

    value["code"] = 0;
    value["data"]["app"] = stream->GetApp();
    value["data"]["stream"] = stream->GetStreamID();
    value["data"]["ip"] = "ip test";
    value["data"]["flv"] = "http://" + serverInfo->IP + ":" + to_string(serverInfo->HttpServerPort) + "/" + stream->GetApp() + "/" + stream->GetStreamID() + ".flv";
    value["data"]["ws_flv"] = "ws://" + serverInfo->IP + ":" + to_string(serverInfo->HttpServerPort) + "/" + stream->GetApp() + "/" + stream->GetStreamID() + ".flv";
    value["data"]["rtc"] = "http://" + serverInfo->IP + ":" + to_string(serverInfo->Port) + SMSServer::instance()->getWebrtcPlayApi() + "?appName=" + stream->GetApp() + "&streamName=" + stream->GetStreamID() + "&enableDtls=1";
    
    value["data"]["rtmp"] = "rtmp://" + serverInfo->IP + ":" + to_string(serverInfo->RtmpPort) + "/" + stream->GetApp() + "/" + stream->GetStreamID();
    value["data"]["rtsp"] = "rtsp://" + serverInfo->IP + ":" + to_string(serverInfo->RtspPort) + "/" + stream->GetApp() + "/" + stream->GetStreamID();



    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void DownStreamApi::listPullStream(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto list = StreamManager::instance()->GetAllStream();
    for (auto &stream : list) {
        if (stream->GetType() != STREAM_TYPE_PROXY) {
            continue;
        }

        json item;
        item["type"] = "type";
        item["app"] = stream->GetApp();
        item["stream"] = stream->GetStreamID();
        item["enable"] = 1;
        item["enableAudio"] = 1;
        item["mediaServerId"] = stream->getServerId();

        value["data"]["list"].push_back(item);
    }
    value["data"]["total"] = list.size();

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void DownStreamApi::listAllStream(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto list = StreamManager::instance()->GetAllStream();
    for (auto &stream : list) {
        json item;
        item["type"] = "type";
        item["app"] = stream->GetApp();
        item["stream"] = stream->GetStreamID();
        item["protocol"] = stream->getProtocol();
        item["enable"] = 1;
        item["enableAudio"] = 1;
        item["mediaServerId"] = stream->getServerId();

        value["data"]["list"].push_back(item);
    }
    value["data"]["total"] = list.size();

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}