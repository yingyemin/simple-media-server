#include "Common/ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "WorkPoller/WorkLoopPool.h"
#include "TestApi.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void TestApi::initApi()
{
    g_mapApi.emplace("/api/v1/test/work/task/add", TestApi::addWorkTask);
    g_mapApi.emplace("/api/v1/test/work/ordertask/add", TestApi::addWorkOrderTask);
}

void TestApi::addWorkTask(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    WorkTask::Ptr task = make_shared<WorkTask>();
    task->priority_ = 100;
    task->func_ = [](){
        logInfo << "add a work task";
    };
    WorkLoopPool::instance()->addTask(task);

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void TestApi::addWorkOrderTask(const HttpParser& parser, const UrlParser& urlParser, 
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
