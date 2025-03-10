#ifdef ENABLE_FFMPEG

#include "ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "WorkPoller/WorkLoopPool.h"
#include "FfmpegApi.h"
#include "Ffmpeg/TranscodeTask.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void FfmpegApi::initApi()
{
    g_mapApi.emplace("/api/v1/ffmpeg/task/add", FfmpegApi::addTask);
    g_mapApi.emplace("/api/v1/ffmpeg/task/del", FfmpegApi::delTask);
    g_mapApi.emplace("/api/v1/ffmpeg/task/reconfig", FfmpegApi::reconfig);
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

#endif