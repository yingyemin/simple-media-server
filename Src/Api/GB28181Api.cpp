#include "GB28181Api.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "GB28181/GB28181Server.h"
#include "GB28181/GB28181ClientPull.h"
#include "GB28181/GB28181ClientPush.h"
#include "ApiUtil.h"

// #include <variant>

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void GB28181Api::initApi()
{
    g_mapApi.emplace("/api/v1/gb28181/recv/create", GB28181Api::createGB28181Receiver);
    g_mapApi.emplace("/api/v1/gb28181/send/create", GB28181Api::createGB28181Sender);
    g_mapApi.emplace("/api/v1/gb28181/recv/stop", GB28181Api::stopGB28181Receiver);
    g_mapApi.emplace("/api/v1/gb28181/send/stop", GB28181Api::stopGB28181Sender);
}

void GB28181Api::createGB28181Receiver(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"active", "ssrc", "port", "socketType", "streamName", "appName", "ip"});

    int active = toInt(parser._body["active"]);

    // if (active) {
    //     int ssrc = toInt(parser._body["ssrc"]);
    //     int port = toInt(parser._body["port"]);
    //     int socketType = toInt(parser._body["socketType"]); //1:tcp,2:udp,3:both
    //     string streamName = parser._body["streamName"];
    //     string appName = parser._body["appName"];
    //     string ip = parser._body["ip"];

    //     auto pull = make_shared<GB28181ClientPull>(ip, port, appName, streamName, ssrc, socketType);
    //     pull->create();
    //     pull->start();

    //     value["port"] = pull->getLocalPort();
    //     value["ip"] = pull->getLocalIp();
    //     value["ssrc"] = ssrc;
    // } else {
    //     int newServer = toInt(parser._body["newServer"]);
    //     if (newServer) {
    //         int ssrc = toInt(parser._body["ssrc"]);
    //         int port = toInt(parser._body["port"]);
    //         int socketType = toInt(parser._body["socketType"]); //1:tcp,2:udp,3:both
    //         string streamName = parser._body["streamName"];

    //         GB28181Server::instance()->startReceive("0.0.0.0", port, socketType);

    //         if (ssrc) {
    //             // add ssrc map streamname
    //         }
    //         value["port"] = port;
    //         value["ip"] = Config::instance()->get("LocalIp");
    //         value["ssrc"] = ssrc;
    //     } else {
    //         // add ssrc map streamname
    //         int ssrc = toInt(parser._body["ssrc"]);
    //         string gbServerName = parser._body.value("gbServerName", "Server1");
    //         value["port"] = Config::instance()->get("GB28181", "Server", gbServerName, "port");
    //         value["ip"] = Config::instance()->get("LocalIp");
    //         value["ssrc"] = ssrc;
    //     }
    // }

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void GB28181Api::createGB28181Sender(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    checkArgs(parser._body, {"active", "ssrc", "port", "socketType", "streamName", "appName", "ip"});

    int active = toInt(parser._body["active"]);
    // logInfo << "json active: " << parser._body["active"];
    // logInfo << "active: " << active;
    if (active) {
        int ssrc = toInt(parser._body["ssrc"]);
        int port = toInt(parser._body["port"]);
        int socketType = toInt(parser._body["socketType"]); //1:tcp,2:udp,3:both
        string streamName = parser._body["streamName"];
        string appName = parser._body["appName"];
        string ip = parser._body["ip"];

        auto push = make_shared<GB28181ClientPush>(ip, port, appName, streamName, ssrc, socketType);
        push->create();
        push->start();

        string key = ip + "_" + to_string(port) + "_" + to_string(ssrc);
        GB28181Client::addClient(key, push);

        value["port"] = push->getLocalPort();
        value["ip"] = push->getLocalIp();
        value["ssrc"] = ssrc;
    } else {
        int ssrc = toInt(parser._body["ssrc"]);
        int port = toInt(parser._body["port"]);
        int socketType = toInt(parser._body["socketType"]); //1:tcp,2:udp,3:both
        string streamName = parser._body["streamName"];
        string appName = parser._body["appName"];

        GB28181Server::instance()->startSend("0.0.0.0", port, socketType, appName, streamName, ssrc);

        value["port"] = port;
        value["ip"] = Config::instance()->get("LocalIp");
        value["ssrc"] = ssrc;
    }

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void GB28181Api::stopGB28181Receiver(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    
}

void GB28181Api::stopGB28181Sender(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    
}

