#include "Http/ApiUtil.h"
#include "Logger.h"
#include "VodApi.h"
#include "Common/MediaSource.h"
#include "Common/Define.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void VodApi::initApi()
{
    g_mapApi.emplace("/api/v1/vod/start", VodApi::start);
    g_mapApi.emplace("/api/v1/vod/control", VodApi::control);
    g_mapApi.emplace("/api/v1/vod/stop", VodApi::stop);
}

void VodApi::start(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"uri", "format", "startTime", "endTime"});

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void VodApi::stop(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"uri"});
    
    std::string uri = parser._body["uri"];
    auto source = MediaSource::get(uri, DEFAULT_VHOST);
    if (source) {
        auto reader = source->getReader();
        if (reader) {
            reader->stop();
        }
    }
    
    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void VodApi::control(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"uri"});

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    
    std::string uri = parser._body["uri"];
    auto source = MediaSource::get(uri, DEFAULT_VHOST);
    if (!source) {
        value["code"] = "404";
        value["msg"] = "source is empty";
        rsp.setContent(value.dump());
        rspFunc(rsp);

        return ;
    }

    auto reader = source->getReader();
    if (!reader) {
        value["code"] = "404";
        value["msg"] = "reader is empty";
        rsp.setContent(value.dump());
        rspFunc(rsp);

        return ;
    }

    float scale = getFloat(parser._body, "scale", 0);
    if (scale > 0.01) {
        reader->scale(scale);
    }

    int seek = getInt(parser._body, "seek", 0);
    if (seek > 0) {
        reader->seek(seek);
    }

    int pause = getInt(parser._body, "pause", 0);
    reader->pause(pause);

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}