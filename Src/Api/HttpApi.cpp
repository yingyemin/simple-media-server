#include "HttpApi.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Common/MediaSource.h"
#include "Util/String.h"
#include "Util/Thread.h"
#include "EventPoller/EventLoopPool.h"
#include "Common/Define.h"
#include "ApiUtil.h"

using namespace std;

unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void HttpApi::route(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    string msg = "unknwon api";
    auto it = g_mapApi.find(urlParser.path_);
    try {
        if (it != g_mapApi.end()) {
            it->second(parser, urlParser, rspFunc);
            return ;
        }
    } catch (ApiException& ex) {
        logInfo << urlParser.path_ << " error: " << ex.what();
        msg = ex.what();
    } catch (exception& ex) {
        logInfo << urlParser.path_ << " error: " << ex.what();
        msg = ex.what();
    } catch (...) {
        logInfo << urlParser.path_ << " error";
    }

    HttpResponse rsp;
    rsp._status = 400;
    json value;
    value["msg"] = msg;
    rsp.setContent(value.dump());
    rspFunc(rsp);
    
}

void HttpApi::initApi()
{
    g_mapApi.emplace("/api/v1/config", HttpApi::handleConfig);
    g_mapApi.emplace("/api/v1/onStreamStatus", HttpApi::onStreamStatus);
    g_mapApi.emplace("/api/v1/onNonePlayer", HttpApi::onNonePlayer);
    g_mapApi.emplace("/api/v1/getSourceList", HttpApi::getSourceList);
    g_mapApi.emplace("/api/v1/getSourceInfo", HttpApi::getSourceInfo);
    g_mapApi.emplace("/api/v1/getLoopList", HttpApi::getLoopList);
    g_mapApi.emplace("/api/v1/exitServer", HttpApi::exitServer);
}

void HttpApi::handleConfig(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    if (parser._method == "GET") {
        logInfo << "get a config api";
        HttpResponse rsp;
        rsp._status = 200;
        auto config = Config::instance()->getConfig();
        rsp.setContent(config.dump());
        rspFunc(rsp);
    } else if (parser._method == "POST") {
        json value = parser._body;
        logInfo << value.dump();
        
        logInfo << value.value("Value", "");
        Config::instance()->setAndUpdate(value.value("Value", ""), value.value("Key1", ""), 
                    value.value("Key2", ""), value.value("Key3", ""), value.value("Key4", ""));
        
        HttpResponse rsp;
        rsp._status = 200;
        rsp.setContent("config api");
        rspFunc(rsp);
    } else {
        HttpResponse rsp;
        rsp._status = 400;
        json value;
        value["msg"] = "unknwon api";
        rsp.setContent(value.dump());
        rspFunc(rsp);
    }
}

void HttpApi::onStreamStatus(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HttpApi::onNonePlayer(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    static int stopNonePlayerStream = Config::instance()->getAndListen([](const json& config){
        stopNonePlayerStream = Config::instance()->get("Util", "stopNonePlayerStream");
    }, "Util", "stopNonePlayerStream");

    static int nonePlayerWaitTime = Config::instance()->getAndListen([](const json& config){
        nonePlayerWaitTime = Config::instance()->get("Util", "nonePlayerWaitTime");
    }, "Util", "nonePlayerWaitTime");

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    value["stop"] = stopNonePlayerStream;
    value["delay"] = nonePlayerWaitTime;
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HttpApi::getSourceList(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto totalSource = MediaSource::getAllSource();
    for (auto& iter : totalSource) {
        auto source = iter.second;
        json item;
        item["path"] = source->getPath();
        item["type"] = source->getType();
        item["protocol"] = source->getProtocol();
        item["vhost"] = source->getVhost();

        auto loop = source->getLoop();
        item["epollFd"] = loop->getEpollFd();

        auto muxerSource = source->getMuxerSource();
        for (auto& mIt : muxerSource) {
            auto tSource = mIt.second;
            for (auto& tIt: tSource) {
                auto mSource = tIt.second.lock();
                if (!mSource) {
                    continue;
                }

                json mItem;
                mItem["protocol"] = mSource->getProtocol();
                mItem["type"] = mSource->getType();

                item.push_back(mItem);
            }
        }

        value["sources"].push_back(item);
    }

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HttpApi::getSourceInfo(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    json body = parser._body;

    auto source = MediaSource::get(body["path"], body.value("vhost", DEFAULT_VHOST));
    if (source) {
        value["path"] = source->getPath();
        value["type"] = source->getType();
        value["protocol"] = source->getProtocol();
        value["vhost"] = source->getVhost();

        auto loop = source->getLoop();
        value["epollFd"] = loop->getEpollFd();

        auto muxerSource = source->getMuxerSource();
        for (auto& mIt : muxerSource) {
            auto tSource = mIt.second;
            for (auto& tIt: tSource) {
                auto mSource = tIt.second.lock();
                if (!mSource) {
                    continue;
                }

                json mItem;
                mItem["protocol"] = mSource->getProtocol();
                mItem["type"] = mSource->getType();

                value.push_back(mItem);
            }
        }
        value["code"] = "200";
        value["msg"] = "success";
    } else {
        rsp._status = 404;
        value["code"] = "404";
        value["msg"] = "source is not exist";
    }

    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HttpApi::getLoopList(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    EventLoopPool::instance()->for_each_loop([&value](const EventLoop::Ptr &loop){
        json item;
        item["epollFd"] = loop->getEpollFd();
        item["fdCount"] = loop->getFdCount();
        item["timerTaskCount"] = loop->getTimerTaskCount();

        int lastWaitDuration, lastRunDuration, curWaitDuration, curRunDuration;
        loop->getLoad(lastWaitDuration, lastRunDuration, curWaitDuration, curRunDuration);
        item["lastWaitDuration"] = lastWaitDuration;
        item["lastRunDuration"] = lastRunDuration;
        item["curWaitDuration"] = curWaitDuration;
        item["curRunDuration"] = curRunDuration;

        value["loops"].push_back(item);
    });

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HttpApi::exitServer(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    exit(0);
}