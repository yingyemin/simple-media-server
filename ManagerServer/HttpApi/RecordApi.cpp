#include "RecordApi.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Common/ApiUtil.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void RecordApi::initApi()
{
    g_mapApi.emplace("/api/v1/cloud/record/list", RecordApi::list);
    g_mapApi.emplace("/api/v1/cloud/record/date/list", RecordApi::listDate);
    g_mapApi.emplace("/api/v1/cloud/record/task/add", RecordApi::addTask);
    g_mapApi.emplace("/api/v1/cloud/record/task/list", RecordApi::listTask);
    g_mapApi.emplace("/api/v1/cloud/record/play/path", RecordApi::playPath);
    g_mapApi.emplace("/api/v1/cloud/collect/add", RecordApi::addCollect);
    g_mapApi.emplace("/api/v1/cloud/collect/delete", RecordApi::delCollect);
}

void RecordApi::list(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    // checkArgs(parser._body, { "username", "password" });

    int page = getInt(parser._body, "page", 1);
    page = page > 0 ? page - 1 : 0;
    int count = getInt(parser._body, "count", 15);

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    // auto records = CloudRecordTb::instance()->listCloudRecord(page, count);
    // value["data"]["total"] = records.size();

    // for (auto record : records) {
    //     json item;
    //     item["id"] = record.id;
    //     item["app"] = record.app;
    //     item["stream"] = record.stream;
    //     item["callId"] = record.callId;
    //     item["startTime"] = record.startTime;
    //     item["endTime"] = record.endTime;
    //     item["mediaServerId"] = record.mediaServerId;
    //     item["fileName"] = record.fileName;
    //     item["filePath"] = record.filePath;
    //     item["folder"] = record.folder;
    //     item["collect"] = record.collect;
    //     // item["reserve"] = record.reserve;
    //     item["fileSize"] = record.fileSize;
    //     item["timeLen"] = record.timeLen;

    //     value["data"]["list"].push_back(item);
    // }

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RecordApi::listDate(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    // checkArgs(parser._body, { "username", "password" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    // auto records = CloudRecordTb::instance()->listCloudRecord(1, 15);

    // for (auto record : records) {
    //     value["data"].push_back(to_string(record.timeLen));
    // }

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RecordApi::addTask(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    // checkArgs(parser._body, { "username", "password" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RecordApi::listTask(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    // checkArgs(parser._body, { "username", "password" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RecordApi::playPath(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "recordId" });
    int recordId = getInt(parser._body, "recordId");

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    // auto record = CloudRecordTb::instance()->getCloudRecord(recordId);

    // value["data"]["httpPath"] = "http://" + record->mediaServerId + record->fileName;
    // value["data"]["httpsPath"] = "https://" + record->mediaServerId + record->fileName;

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RecordApi::addCollect(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    // checkArgs(parser._body, { "username", "password" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void RecordApi::delCollect(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    // checkArgs(parser._body, { "username", "password" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}