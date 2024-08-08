#include "ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "WorkPoller/WorkLoopPool.h"
#include "RecordApi.h"
#include "Record/RecordPs.h"
#include "Common/Define.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void RecordApi::initApi()
{
    g_mapApi.emplace("/api/v1/record/start", RecordApi::startRecord);
    g_mapApi.emplace("/api/v1/record/stop", RecordApi::stopRecord);
}

void RecordApi::startRecord(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"appName", "streamName"});
    UrlParser recordUrlParser;
    recordUrlParser.path_ = "/" + parser._body["appName"].get<string>() + "/" + parser._body["streamName"].get<string>();
    recordUrlParser.protocol_ = PROTOCOL_PS;
    recordUrlParser.vhost_ = DEFAULT_VHOST;
    recordUrlParser.type_ = DEFAULT_TYPE;
    Record::Ptr record = make_shared<RecordPs>(recordUrlParser);

    string taskId = randomStr(8);
    Record::addRecord(recordUrlParser.path_, taskId, record);

    record->setOnClose([recordUrlParser, taskId](){
        Record::delRecord(recordUrlParser.path_, taskId);
    });

    record->start();

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["taskId"] = taskId;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RecordApi::stopRecord(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"appName", "streamName", "taskId"});
    string path = "/" + parser._body["appName"].get<string>() + "/" + parser._body["streamName"].get<string>();
    auto record = Record::getRecord(path, parser._body["taskId"]);
    record->stop();

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}
