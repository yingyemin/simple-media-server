#ifdef ENABLE_FFMPEG

#include "Http/ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.hpp"
#include "WorkPoller/WorkLoopPool.h"
#include "FfmpegApi.h"
#include "Ffmpeg/TranscodeTask.h"
#include "Ffmpeg/VideoStack.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void FfmpegApi::initApi()
{
    g_mapApi.emplace("/api/v1/ffmpeg/task/add", FfmpegApi::addTask);
    g_mapApi.emplace("/api/v1/ffmpeg/task/del", FfmpegApi::delTask);
    g_mapApi.emplace("/api/v1/ffmpeg/task/reconfig", FfmpegApi::reconfig);
    
    g_mapApi.emplace("/api/v1/video/stack/start", FfmpegApi::startVideoStack);
    g_mapApi.emplace("/api/v1/video/stack/custom/start", FfmpegApi::startCustomVideoStack);
    g_mapApi.emplace("/api/v1/video/stack/reset", FfmpegApi::resetVideoStack);
    g_mapApi.emplace("/api/v1/video/stack/stop", FfmpegApi::stopVideoStack);
}

void FfmpegApi::addTask(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"path"});

    string videoCodec;
    string audioCodec;

    if (parser._body.find("videoCodec") != parser._body.end()) {
        videoCodec = parser._body["videoCodec"];
    }

    if (parser._body.find("audioCodec") != parser._body.end()) {
        audioCodec = parser._body["audioCodec"];
    }

    if (videoCodec.empty() && audioCodec.empty()) {
        throw ApiException(400, "videoCodec or audioCodec must be set");
    }

    string taskId = TranscodeTask::addTask(parser._body["path"], videoCodec, audioCodec);

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    value["taskId"] = taskId;
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void FfmpegApi::delTask(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"taskId"});

    TranscodeTask::delTask(parser._body["taskId"]);

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void FfmpegApi::reconfig(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"taskId"});

    auto task = TranscodeTask::getTask(parser._body["taskId"]);
    if (!task) {
        throw ApiException(404, "task not found");
    }

    if (parser._body.find("bitrate") != parser._body.end()) {
        task->setBitrate(toInt(parser._body["bitrate"]));
    }

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void FfmpegApi::startVideoStack(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    VideoStackManager::Instance().startVideoStack(parser._body);

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void FfmpegApi::startCustomVideoStack(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"streams", "path", "width", "height"});
    std::string path = parser._body["path"];
    int width = toInt(parser._body["width"]);
    int height = toInt(parser._body["height"]);
    std::string bgImg = parser._body.value("bgImg", "");
    auto params = std::make_shared<std::vector<Param::Ptr>>();
    json streams = json::parse(parser._body["streams"].get<string>());
    for (auto& stream : streams) {
        checkArgs(stream, {"id", "posX", "posY", "width", "height"});
        Param::Ptr p = std::make_shared<Param>();
        p->id = stream["id"];
        p->bgImg = stream.value("bgImg", "");
        p->posX = toInt(stream["posX"]);
        p->posY = toInt(stream["posY"]);
        p->width = toInt(stream["width"]);
        p->height = toInt(stream["height"]);
        if (stream.find("pixfmt") != stream.end()) {
            p->pixfmt = (AVPixelFormat)toInt(stream["pixfmt"]);
        }

        params->push_back(p);
    }
    
    VideoStackManager::Instance().startCustomVideoStack(path, width, height, bgImg, params);

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void FfmpegApi::stopVideoStack(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"taskId"});

    VideoStackManager::Instance().stopVideoStack(parser._body["taskId"]);

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void FfmpegApi::resetVideoStack(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    VideoStackManager::Instance().resetVideoStack(parser._body);

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

#endif