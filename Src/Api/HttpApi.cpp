#include "HttpApi.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Common/FrameMediaSource.h"
#include "Util/String.h"
#include "Util/Thread.h"
#include "EventPoller/EventLoopPool.h"
#include "Common/Define.h"
#include "Common/ApiUtil.h"
#include "Util/TimeClock.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void HttpApi::initApi()
{
    g_mapApi.emplace("/api/v1/config", HttpApi::handleConfig);
    g_mapApi.emplace("/api/v1/getSourceList", HttpApi::getSourceList);
    g_mapApi.emplace("/api/v1/getSourceInfo", HttpApi::getSourceInfo);
    g_mapApi.emplace("/api/v1/closeSource", HttpApi::closeSource);
    g_mapApi.emplace("/api/v1/getClientList", HttpApi::getClientList);
    g_mapApi.emplace("/api/v1/closeClient", HttpApi::closeClient);
    g_mapApi.emplace("/api/v1/getLoopList", HttpApi::getLoopList);
    g_mapApi.emplace("/api/v1/exitServer", HttpApi::exitServer);
    g_mapApi.emplace("/api/v1/version", HttpApi::getVersion);
    g_mapApi.emplace("/api/v1/getServerInfo", HttpApi::getServerInfo);

    g_mapApi.emplace("/api/v1/streams/keyframe", HttpApi::getKeyframe);
    g_mapApi.emplace("/api/v1/streams/setStampMode", HttpApi::setStampMode);
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

void HttpApi::getSourceList(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto totalSource = MediaSource::getAllSource();
    value["count"] = totalSource.size();
    for (auto& iter : totalSource) {
        auto source = iter.second;
        if (!source) {
            continue;
        }
        json item;
        item["path"] = source->getPath();
        item["type"] = source->getType();
        item["protocol"] = source->getProtocol();
        item["vhost"] = source->getVhost();

        auto loop = source->getLoop();
        if (loop) {
            item["epollFd"] = loop->getEpollFd();
        }
        
        item["playerCount"] = source->playerCount();
        item["bytes"] = source->getBytes();
        item["createTime"] = source->getCreateTime();
        item["statuc"] = source->isReady() ? "ready" : "unready";
        item["onlineDuration"] = TimeClock::now() - source->getCreateTime();
        item["action"] = source->getAction() ? "push" : "pull";
        auto tracks = source->getTrackInfo();
        for (auto iter : tracks) {
            string status = iter.second->isReady() ? "ready" : "expire";
            if (iter.second->trackType_ == "video") {
                item["video"]["codec"] = iter.second->codec_;
                int height = 0;
                int width = 0;
                int fps = 0;
                iter.second->getWidthAndHeight(width, height, fps);
                item["video"]["width"] = width;
                item["video"]["height"] = height;
                item["video"]["fps"] = fps;
                item["video"]["status"] = status;
            } else if (iter.second->trackType_ == "audio") {
                item["audio"]["codec"] = iter.second->codec_;
                item["audio"]["status"] = status;
            }
        }
        auto sock = source->getOriginSocket();
        if (sock) {
            item["socketInfo"]["localIp"] = sock->getLocalIp();
            item["socketInfo"]["localPort"] = sock->getLocalPort();
            item["socketInfo"]["peerIp"] = sock->getPeerIp();
            item["socketInfo"]["peerPort"] = sock->getPeerPort();
        }
        int totalPlayerCount = source->playerCount();

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
                mItem["playerCount"] = mSource->playerCount();
                if (mSource->getProtocol() == "frame") {
                    auto frameSrc = dynamic_pointer_cast<FrameMediaSource>(mSource);
                    mItem["video"]["fps"] = frameSrc->getFps();
                    mItem["video"]["lastFrameTime"] = frameSrc->getLastFrameTime();
                    mItem["video"]["lastGopTime"] = frameSrc->getLastGopTime();
                    mItem["video"]["lastKeyframeTime"] = frameSrc->getLastKeyframeTime();
                }
                totalPlayerCount += mSource->playerCount();

                item["muxer"].push_back(mItem);
            }
        }

        item["totalPlayerCount"] = totalPlayerCount;
        value["sources"].push_back(item);
    }

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HttpApi::closeSource(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    json body = parser._body;
    checkArgs(body, {"path"});

    auto source = MediaSource::get(body["path"], body.value("vhost", DEFAULT_VHOST));
    if (source) {
        value["code"] = "200";
        value["msg"] = "success";

        auto loop = source->getLoop();
        if (loop) {
            weak_ptr<MediaSource> weakSelf = source;
            loop->async([weakSelf](){
                auto source = weakSelf.lock();
                if (source) {
                    source->release();
                }
            }, true);
        }
    } else {
        rsp._status = 404;
        value["code"] = "404";
        value["msg"] = "source is not exist";
    }

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
    checkArgs(body, {"path"});

    auto source = MediaSource::get(body["path"], body.value("vhost", DEFAULT_VHOST));
    if (source) {
        value["path"] = source->getPath();
        value["type"] = source->getType();
        value["protocol"] = source->getProtocol();
        value["vhost"] = source->getVhost();
        value["playerCount"] = source->playerCount();
        value["bytes"] = source->getBytes();
        value["createTime"] = source->getCreateTime();
        value["onlineDuration"] = TimeClock::now() - source->getCreateTime();
        value["action"] = source->getAction() ? "push" : "pull";
        auto tracks = source->getTrackInfo();
        for (auto iter : tracks) {
            string status = iter.second->isReady() ? "ready" : "expire";
            if (iter.second->trackType_ == "video") {
                value["video"]["codec"] = iter.second->codec_;
                int height = 0;
                int width = 0;
                int fps = 0;
                iter.second->getWidthAndHeight(width, height, fps);
                value["video"]["width"] = width;
                value["video"]["height"] = height;
                value["video"]["fps"] = fps;
                value["video"]["status"] = status;
            } else if (iter.second->trackType_ == "audio") {
                value["audio"]["codec"] = iter.second->codec_;
                value["audio"]["status"] = status;
            }
        }
        auto sock = source->getOriginSocket();
        if (sock) {
            value["socketInfo"]["localIp"] = sock->getLocalIp();
            value["socketInfo"]["localPort"] = sock->getLocalPort();
            value["socketInfo"]["peerIp"] = sock->getPeerIp();
            value["socketInfo"]["peerPort"] = sock->getPeerPort();
        }
        
        int totalPlayerCount = source->playerCount();

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
                mItem["playerCount"] = mSource->playerCount();
                if (mSource->getProtocol() == "frame") {
                    auto frameSrc = dynamic_pointer_cast<FrameMediaSource>(mSource);
                    mItem["video"]["fps"] = frameSrc->getFps();
                    mItem["video"]["lastFrameTime"] = frameSrc->getLastFrameTime();
                    mItem["video"]["lastGopTime"] = frameSrc->getLastGopTime();
                    mItem["video"]["lastKeyframeTime"] = frameSrc->getLastKeyframeTime();
                }
                totalPlayerCount += mSource->playerCount();

                value["muxer"].push_back(mItem);
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

void HttpApi::getClientList(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    json body = parser._body;
    checkArgs(body, {"path", "protocol"});

    auto source = MediaSource::get(body["path"], body.value("vhost", DEFAULT_VHOST), body["protocol"], DEFAULT_TYPE);
    if (source) {
        value["path"] = source->getPath();
        value["type"] = source->getType();
        value["protocol"] = source->getProtocol();
        value["vhost"] = source->getVhost();
        value["playerCount"] = source->playerCount();

        auto loop = source->getLoop();
        value["epollFd"] = loop->getEpollFd();
        value["code"] = "200";
        value["msg"] = "success";

        source->getClientList([value, rsp, rspFunc](const list<ClientInfo> &list){
            for (auto& data : list) {
                json client;
                client["ip"] = data.ip_;
                client["port"] = data.port_;
                client["bitrate"] = data.bitrate_;

                const_cast<json &>(value)["client"].push_back(client);
            }
            const_cast<HttpResponse &>(rsp).setContent(value.dump());
            rspFunc(const_cast<HttpResponse &>(rsp));
        });

        return ;
    } else {
        rsp._status = 404;
        value["code"] = "404";
        value["msg"] = "source is not exist";
    }

    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HttpApi::closeClient(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    json body = parser._body;
    checkArgs(body, {"path", "protocol", "clientIp", "clientPort"});

    auto source = MediaSource::get(body["path"], body.value("vhost", DEFAULT_VHOST), body["protocol"], DEFAULT_TYPE);
    if (source) {
        value["code"] = "200";
        value["msg"] = "success";

        source->getClientList([value, rsp, rspFunc, body](const list<ClientInfo> &list){
            for (auto& data : list) {
                if (data.ip_ == body["clientIp"].get<string>() && data.port_ == toInt(body["clientPort"])) {
                    if (data.close_) {
                        data.close_();
                    }
                    break;
                }
            }
            const_cast<HttpResponse &>(rsp).setContent(value.dump());
            rspFunc(const_cast<HttpResponse &>(rsp));
        });

        return ;
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

void HttpApi::getServerInfo(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    value["eventLoop"]["threadSize"] = EventLoopPool::instance()->getThreadSize();
    value["eventLoop"]["startTime"] = EventLoopPool::instance()->getStartTime();

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HttpApi::getVersion(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    value["version"] = GIT_VERSION;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HttpApi::setStampMode(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    json body = parser._body;
    checkArgs(body, {"path"});
    
    value["code"] = "200";
    value["msg"] = "success";

    auto source = MediaSource::get(body["path"], body.value("vhost", DEFAULT_VHOST), PROTOCOL_FRAME, body.value("type", DEFAULT_TYPE));
    if (!source) {
        value["code"] = "400";
        value["msg"] = "source is empty";
    } else {
        if (body.find("videoStampMode") != body.end()) {
            StampMode videoMode = body["videoStampMode"];
            source->setVideoStampMode(videoMode);
        }
        if (body.find("audioStampMode") != body.end()) {
            StampMode audioMode = body["audioStampMode"];
            source->setAudioStampMode(audioMode);
        }
    }

    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HttpApi::getKeyframe(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    json body = parser._body;
    checkArgs(body, {"path"});

    do {
        auto source = MediaSource::get(body["path"], body.value("vhost", DEFAULT_VHOST), PROTOCOL_FRAME, body.value("type", DEFAULT_TYPE));
        if (!source) {
            value["code"] = "400";
            value["msg"] = "source is empty";
            break;
        }

        auto frameSrc = dynamic_pointer_cast<FrameMediaSource>(source);
        if (!frameSrc) {
            value["code"] = "400";
            value["msg"] = "source is not frame source";
            break;
        }

        if (!frameSrc->isReady()) {
            value["code"] = "400";
            value["msg"] = "frame source is ready";
            break;
        }
        
        auto curLoop = EventLoop::getCurrentLoop();
        auto sourceLoop = frameSrc->getLoop();
        if (!curLoop || !sourceLoop) {
            value["code"] = "400";
            value["msg"] = "loop is empty";
            break;
        }
        sourceLoop->async([frameSrc, curLoop, rsp, value, rspFunc](){
            auto keyframe = frameSrc->getKeyframe();
            auto trackInfos = frameSrc->getTrackInfo();
            for (auto track: trackInfos) {
                if (track.second->trackType_ == "video") {
                    FrameBuffer::Ptr vps;
                    FrameBuffer::Ptr sps;
                    FrameBuffer::Ptr pps;
                    track.second->getVpsSpsPps(vps, sps, pps);
                    string keyframeStr;
                    if (vps) {
                        keyframeStr.append(vps->data(), vps->size());
                    }
                    if (sps) {
                        keyframeStr.append(sps->data(), sps->size());
                    }
                    if (pps) {
                        keyframeStr.append(pps->data(), pps->size());
                    }
                    if (keyframe) {
                        keyframeStr.append(keyframe->data(), keyframe->size());
                    }

                    curLoop->async([keyframeStr, rsp, rspFunc]() {
                        auto tmpRsp = rsp;
                        tmpRsp.setContent(keyframeStr, "application/octet-stream");
                        rspFunc(tmpRsp);
                    }, true);
                }
            }
        }, true);

        return;
    } while(0);

    rsp.setContent(value.dump());
    rspFunc(rsp);
}