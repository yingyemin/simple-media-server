#include "Http/ApiUtil.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.hpp"
#include "GB28181SIPApi.h"
#include "GB28181SIP/GB28181SIPManager.h"
#include "GB28181SIP/GB28181DeviceManager.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void GB28181SIPApi::initApi()
{
    g_mapApi.emplace("/api/v1/device/catalog", GB28181SIPApi::catalog);
    g_mapApi.emplace("/api/v1/device/invite", GB28181SIPApi::invite);
    g_mapApi.emplace("/api/v1/device/list", GB28181SIPApi::listDevice);
    g_mapApi.emplace("/api/v1/device/info", GB28181SIPApi::getDevice);
}

void GB28181SIPApi::catalog(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"deviceId"});
    auto deviceCtx = GB28181SIPManager::instance()->getContext(parser._body["deviceId"]);
    deviceCtx->catalog();

    HttpResponse rsp;
    rsp._status = 200;
    json value;
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void GB28181SIPApi::invite(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"deviceId", "channelId", "channelNum", "ip", "port", "ssrc"});

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    MediaInfo info;
    info.channelId = parser._body["channelId"];
    info.channelNum = toInt(parser._body["channelNum"]);
    info.ip = parser._body["ip"];
    info.port = toInt(parser._body["port"]);
    info.ssrc = toInt(parser._body["ssrc"]);

    auto deviceCtx = GB28181SIPManager::instance()->getContext(parser._body["deviceId"]);
    if (!deviceCtx) {
        value["code"] = "400";
        value["msg"] = "device is not exist";
        rsp.setContent(value.dump());
        rspFunc(rsp);
    }
    GB28181SIPManager::instance()->addContext(info.channelId, deviceCtx);
    deviceCtx->invite(info);
    
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void GB28181SIPApi::listDevice(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto deviceList = GB28181DeviceManager::instance()->getMapContext();
    for (auto& deviceWptr : deviceList) {
        auto deviceCtx = deviceWptr.second.lock();
        if (!deviceCtx) {
            continue;
        }
        json item;
        item["name"] = deviceCtx->getDeviceInfo()->_name;
        item["deviceId"] = deviceCtx->getDeviceInfo()->_device_id;
        item["channelSum"] = deviceCtx->getDeviceInfo()->_channel_count;
        auto channelList = deviceCtx->getChannelList();
        item["channelNum"] = channelList.size();
        for (auto& channel : channelList) {
            json itChannel;
            itChannel["channelId"] = channel.second->GetChannelID();

            item["channelList"].push_back(itChannel);
        }
        value["data"].push_back(item);
    }
    
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void GB28181SIPApi::getDevice(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"deviceId"});
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto deviceCtx = GB28181DeviceManager::instance()->getDevice(parser._body["deviceId"]);
    if (!deviceCtx) {
        value["code"] = "400";
        value["msg"] = "device is not exist";
        rsp.setContent(value.dump());
        rspFunc(rsp);

        return;
    }

    value["name"] = deviceCtx->getDeviceInfo()->_name;
    value["deviceId"] = deviceCtx->getDeviceInfo()->_device_id;
    value["channelSum"] = deviceCtx->getDeviceInfo()->_channel_count;
    auto channelList = deviceCtx->getChannelList();
    auto mediaInfoList = deviceCtx->getMediaInfoList();
    value["channelNum"] = channelList.size();
    for (auto& channel : channelList) {
        json itChannel;
        itChannel["channelId"] = channel.second->GetChannelID();
        itChannel["mediaNum"] = mediaInfoList[channel.second->GetChannelID()].size();
        for (auto& media : mediaInfoList[channel.second->GetChannelID()]) {
            json itMedia;
            itMedia["callId"] = media.first;
            itMedia["ip"] = media.second.ip;
            itMedia["port"] = media.second.port;
            itMedia["ssrc"] = media.second.ssrc;
            itChannel["mediaList"].push_back(itMedia);
        }

        value["channelList"].push_back(itChannel);
    }
    
    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}
