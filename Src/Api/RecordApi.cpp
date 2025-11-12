#ifdef ENABLE_RECORD

#include "Http/ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.hpp"
#include "WorkPoller/WorkLoopPool.h"
#include "RecordApi.h"
#include "Record/RecordPs.h"
#include "Record/RecordMp4.h"
#include "Record/RecordFlv.h"
#include "Record/RecordHls.h"
#include "Record/RecordReader.h"
#include "Common/Define.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void RecordApi::initApi()
{
    g_mapApi.emplace("/api/v1/record/start", RecordApi::startRecord);
    g_mapApi.emplace("/api/v1/record/list", RecordApi::listRecord);
    g_mapApi.emplace("/api/v1/record/stop", RecordApi::stopRecord);
    g_mapApi.emplace("/api/v1/record/query", RecordApi::queryRecord);

    g_mapApi.emplace("/api/v1/record/file/query", RecordApi::queryRecordFile);
    g_mapApi.emplace("/api/v1/record/file/delete", RecordApi::delRecordFile);
}

void RecordApi::startRecord(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"appName", "streamName", "format"});

    RecordTemplate::Ptr recordTemplate = make_shared<RecordTemplate>(); 
    if (parser._body.find("recordTemplate") != parser._body.end()) {
        auto jsonTemplate = parser._body["recordTemplate"];
        recordTemplate->duration = getInt(jsonTemplate, "duration", 0);
        recordTemplate->segment_duration = getInt(jsonTemplate, "segmentDuration", 5000);
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
    }else if (format == "flv")
    {
        recordUrlParser.protocol_ = PROTOCOL_FLV;
        record = make_shared<RecordFlv>(recordUrlParser, recordTemplate);
    }else if (format == "hls")
    {
        recordUrlParser.protocol_ = PROTOCOL_HLS;
        record = make_shared<RecordHls>(recordUrlParser, recordTemplate);
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
        recordInfo["format"] = record->getFormat();
        recordInfo["status"] = record->getStatus();
        auto temp = record->getTemplate();
        recordInfo["duration"] = temp->duration;
        recordInfo["createTime"] = record->getCreateTime() * 1000;

        value["records"].push_back(recordInfo);
        ++count;
    });

    value["count"] = count;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RecordApi::queryRecord(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    int count = 0;

    // auto record = Record::getRecord(urlParser.path_, parser._body["taskId"]);
    // value["path"] = record->getUrlParser().path_;
    // value["taskId"] = record->getTaskId();
    value["status"] = "playing";
    value["progress"] = 50;

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RecordApi::queryRecordFile(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto recordFiles = RecordReader::getRecordList(parser._body.value("path", ""), 
                                parser._body.value("year", ""), 
                                parser._body.value("month", ""), 
                                parser._body.value("day", ""));

    for (auto& recordFile : recordFiles) {
        json recordList;
        auto path = recordFile.first;
        recordList["path"] = path;
        for (auto& year : recordFile.second) {
            auto strYear = year.first;
            for (auto& month : year.second) {
                auto strMonth = month.first;
                for (auto& day : month.second) {
                    auto strDay = day.first;
                    for (auto& fileId : day.second) {
                        json recordInfo;
                        recordInfo["path"] = path;
                        recordInfo["year"] = strYear;
                        recordInfo["month"] = strMonth;
                        recordInfo["day"] = strDay;
                        recordInfo["fileId"] = fileId;
                        recordInfo["rtmpUrl"] = "rtmp://127.0.0.1/file/{vodId}" + path + "/" + strYear + "/" + strMonth + "/" + strDay + "/" + fileId + "/1";
                        recordList["records"].push_back(recordInfo);
                    }
                }
            }
        }

        value["recordList"].push_back(recordList);
    }

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RecordApi::delRecordFile(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"path", "year", "month", "day", "fileId"});

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    RecordReader::delRecordInfo(parser._body["path"].get<string>(), 
                                parser._body["year"].get<string>(), 
                                parser._body["month"].get<string>(), 
                                parser._body["day"].get<string>(), 
                                parser._body["fileId"].get<string>());

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

#endif