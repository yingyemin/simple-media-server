#ifdef ENABLE_FFMPEG

#include "ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "WorkPoller/WorkLoopPool.h"
#include "FfmpegApi.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void FfmpegApi::initApi()
{
    g_mapApi.emplace("/api/v1/ffmpeg/task/add", FfmpegApi::addTask);
    g_mapApi.emplace("/api/v1/ffmpeg/task/del", FfmpegApi::delTask);
}

void FfmpegApi::addTask(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"path"});

    if (parser._body.find("videoCodec") != parser._body.end()) {

    }

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void FfmpegApi::delTask(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    auto loop = WorkLoopPool::instance()->getLoopByCircle();
    WorkTask::Ptr task = make_shared<WorkTask>();
    task->priority_ = 100;
    task->func_ = [](){
        logInfo << "add a work task";
    };
    loop->addOrderTask(task);

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

#endif