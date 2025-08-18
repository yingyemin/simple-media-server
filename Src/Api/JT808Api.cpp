#ifdef ENABLE_JT808

#include "JT808Api.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Http/ApiUtil.h"
#include "Util/String.h"
#include "JT808/JT808Server.h"
#include "JT808/JT808Manager.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void JT808Api::initApi()
{
    g_mapApi.emplace("/api/v1/jt808/9101", JT808Api::t9101);
    g_mapApi.emplace("/api/v1/jt808/9201", JT808Api::t9201);
    g_mapApi.emplace("/api/v1/jt808/device/list", JT808Api::listDevice);
}

void JT808Api::t9101(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"simcode", "ip", "port", "channel", "streamType", "mediaType"});
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    JT808MediaInfo mediaInfo;
    mediaInfo.simCode = parser._body["simcode"];
    mediaInfo.ip = parser._body["ip"];
    mediaInfo.tcpPort = toInt(parser._body["port"]);
    mediaInfo.channelNum = toInt(parser._body["channel"]);
    mediaInfo.streamType = toInt(parser._body["streamType"]);
    mediaInfo.mediaType = toInt(parser._body["mediaType"]);

    auto context = JT808Manager::instance()->getContext(mediaInfo.simCode);
    if (!context) {
        value["code"] = "1001";
        value["msg"] = "simcode not exist";
        rsp.setContent(value.dump());
        rspFunc(rsp);
        return;
    }

    context->on9101(mediaInfo);

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void JT808Api::t9201(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, {"port"});
    HttpResponse rsp;
    rsp._status = 200;
    json value;


    value["code"] = "200";
    value["msg"] = "success";
    value["authResult"] = true;
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void JT808Api::listDevice(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto contexts = JT808Manager::instance()->getContexts();
    value["code"] = "200";
    value["msg"] = "success";
    value["total"] = contexts.size();
    for (auto& context : contexts) {
        json item;
        auto deviceInfo = context.second->getDeviceInfo();
        item["provinceId"] = deviceInfo.provinceId;
        item["cityId"] = deviceInfo.cityId;
        item["deviceModel"] = deviceInfo.deviceModel;
        item["deviceId"] = deviceInfo.deviceId;
        item["makerId"] = deviceInfo.makerId;
        item["plateNo"] = deviceInfo.plateNo;
        item["deviceIp"] = deviceInfo.deviceIp;
        item["devicePort"] = deviceInfo.devicePort;

        if (deviceInfo.location) {
            item["latitude"] = deviceInfo.location->getLatitude();
            item["longitude"] = deviceInfo.location->getLongitude();
            item["altitude"] = deviceInfo.location->getAltitude();
            item["speed"] = deviceInfo.location->getSpeed();
            item["direction"] = deviceInfo.location->getDirection();
            item["deviceTime"] = deviceInfo.location->getDeviceTime();
            item["warnBit"] = deviceInfo.location->getWarnBit();
            item["statusBit"] = deviceInfo.location->getStatusBit();
        }

        value["devices"].push_back(item);
    }

    rsp.setContent(value.dump());
    rspFunc(rsp);
}

#endif