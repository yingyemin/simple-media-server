#ifdef ENABLE_RECORD

#include "Common/ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "WorkPoller/WorkLoopPool.h"
#include "RecordApi.h"
#include "Record/RecordPs.h"
#include "Record/RecordMp4.h"
#include "Common/Define.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void RecordApi::initApi()
{
    g_mapApi.emplace("/api/v1/record/start", RecordApi::startRecord);
    g_mapApi.emplace("/api/v1/record/list", RecordApi::listRecord);
    g_mapApi.emplace("/api/v1/record/stop", RecordApi::stopRecord);
}

void RecordApi::startRecord(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"appName", "streamName", "format"});

    RecordTemplate::Ptr recordTemplate = make_shared<RecordTemplate>(); 
    if (parser._body.find("recordTemplate") != parser._body.end()) {
        auto jsonTemplate = parser._body["recordTemplate"];
        recordTemplate->duration = getInt(jsonTemplate, "duration", 0);
        recordTemplate->segment_duration = getInt(jsonTemplate, "segmentDuration", 0);
        recordTemplate->segment_count = getInt(jsonTemplate, "segmentCount", 0);
    }

    string format = parser._body["format"];

    UrlParser recordUrlParser;
    recordUrlParser.path_ = "/" + parser._body["appName"].get<string>() + "/" + parser._body["streamName"].get<string>();
    recordUrlParser.vhost_ = DEFAULT_VHOST;
    recordUrlParser.type_ = DEFAULT_TYPE;

    Record::Ptr record;
    if (format == "ps") {
        recordUrlParser.protocol_ = PROTOCOL_PS;
        record = make_shared<RecordPs>(recordUrlParser, recordTemplate);
    } else if (format == "mp4") {
        recordUrlParser.protocol_ = PROTOCOL_MP4;
        record = make_shared<RecordMp4>(recordUrlParser, recordTemplate);
    }
    
    string taskId = randomStr(8);
    record->setTaskId(taskId);
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
    if (record) {
        record->stop();
    }
    
    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RecordApi::listRecord(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    int count = 0;
    Record::for_each_record([&value, &count](const Record::Ptr& record){
        json recordInfo; 
        auto urlParser = record->getUrlParser();
        recordInfo["path"] = urlParser.path_;
        recordInfo["taskId"] = record->getTaskId();

        value["records"].push_back(recordInfo);
        ++count;
    });

    value["count"] = count;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

#endif