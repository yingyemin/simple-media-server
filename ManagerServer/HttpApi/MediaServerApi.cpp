#include "MediaServerApi.h"
#include "Logger.h"
#include "Common/Config.h"
#include "../Common/ApiUtil.h"
#include "Manager/SMSServer.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void MediaServerApi::initApi()
{
    g_mapApi.emplace("/api/v1/media/server/onRegisterServer", MediaServerApi::onRegisterServer);
    g_mapApi.emplace("/api/v1/media/server/onKeepAlive", MediaServerApi::onKeepAlive);
    g_mapApi.emplace("/api/v1/media/server/list", MediaServerApi::list);
    g_mapApi.emplace("/api/v1/media/server/query", MediaServerApi::query);
    g_mapApi.emplace("/api/v1/media/server/check", MediaServerApi::check);
    g_mapApi.emplace("/api/v1/media/server/save", MediaServerApi::save);
    g_mapApi.emplace("/api/v1/media/server/delete", MediaServerApi::del);
    g_mapApi.emplace("/api/v1/media/server/load", MediaServerApi::load);
    g_mapApi.emplace("/api/v1/media/server/get", MediaServerApi::get);
}

void MediaServerApi::onRegisterServer(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "serverId" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto server = SMSServer::instance()->getServer(parser._body["serverId"]);
    if (!server) {
        server = make_shared<SMSServer>();
        auto info = make_shared<MediaServerInfo>();
        server->setInfo(info);
        SMSServer::instance()->addServer(parser._body["serverId"], server);
    }
    auto serverInfo = server->getInfo();
    //流媒体服务 IP
	serverInfo->IP = parser._body.value("ip", "");
	//流媒体服务 端口
	serverInfo->Port = getInt(parser._body, "port", 8000);
	serverInfo->RtpPort = getInt(parser._body, "rtpServerPort", 10000);
	serverInfo->RtmpPort = getInt(parser._body, "rtmpServerPort", 1935);
	serverInfo->RtspPort = getInt(parser._body, "rtspServerPort", 554);
	serverInfo->HttpServerPort = getInt(parser._body, "httpServerPort", 8080);
	serverInfo->JT1078Port = getInt(parser._body, "jt1078Port", 17000);
    serverInfo->ServerId = parser._body["serverId"];
	auto base_url = "http://" + serverInfo->IP + ":" + to_string(serverInfo->Port);
    server->setBaseUrl(base_url);
    server->UpdateHeartbeatTime();

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void MediaServerApi::onKeepAlive(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "serverId" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto server = SMSServer::instance()->getServer(parser._body["serverId"]);
    if (!server) {
        server = make_shared<SMSServer>();
        auto info = make_shared<MediaServerInfo>();
        server->setInfo(info);
        SMSServer::instance()->addServer(parser._body["serverId"], server);
    }
    auto serverInfo = server->getInfo();
	//流媒体服务 端口
	serverInfo->Port = getInt(parser._body, "port", 8000);
	serverInfo->RtpPort = getInt(parser._body, "rtpServerPort", 10000);
	serverInfo->RtmpPort = getInt(parser._body, "rtmpServerPort", 1935);
	serverInfo->RtspPort = getInt(parser._body, "rtspServerPort", 554);
	serverInfo->HttpServerPort = getInt(parser._body, "httpServerPort", 8080);
    serverInfo->JT1078Port = getInt(parser._body, "jt1078Port", 17000);
    serverInfo->ServerId = parser._body["serverId"];
    //流媒体服务 IP
    if (serverInfo->IP.empty()) {
	    serverInfo->IP = parser._body.value("ip", "");
        auto base_url = "http://" + serverInfo->IP + ":" + to_string(serverInfo->Port);
        server->setBaseUrl(base_url);
    }
    server->UpdateHeartbeatTime();

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void MediaServerApi::list(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto serverList = SMSServer::instance()->getServerList();
	int online = getInt(parser._body, "online", 0);
    for (auto server : serverList) {
        if (online && !server.second->IsConnected()) {
            continue;
        }

        json item;
        auto info = server.second->getInfo();
        item["id"] = info->ServerId;
        item["ip"] = info->IP;
        item["port"] = info->Port;
        item["hookIp"] = "hookIp";
        item["sdpIp"] = info->IP;
        item["streamIp"] = info->IP;
        item["httpPort"] = info->HttpServerPort;
        item["jt1078Port"] = info->JT1078Port;
        item["rtmpPort"] = info->RtmpPort;
        item["rtpProxyPort"] = info->RtpPort;
        item["rtspPort"] = info->RtspPort;
        item["hookAliveInterval"] = 0;
        item["rtpEnable"] = 1;
        item["status"] = server.second->IsConnected();
        item["createTime"] = "000";
        item["updateTime"] = "000";
        item["lastKeepaliveTime"] = "000";

        value["data"].push_back(item);
    }

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void MediaServerApi::query(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "serverId" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto server = SMSServer::instance()->getServer(parser._body["serverId"]);
    if (!server) {
        value["code"] = 404;
        value["msg"] = "server not found";
        rsp.setContent(value.dump());
        rspFunc(rsp);

        return ;
    }
    json item;
    auto info = server->getInfo();
    item["id"] = info->ServerId;
    item["ip"] = info->IP;
    item["port"] = info->Port;
    item["hookIp"] = "hookIp";
    item["sdpIp"] = info->IP;
    item["streamIp"] = info->IP;
    item["httpPort"] = info->HttpServerPort;
    item["jt1078Port"] = info->JT1078Port;
    item["rtmpPort"] = info->RtmpPort;
    item["rtpProxyPort"] = info->RtpPort;
    item["rtspPort"] = info->RtspPort;
    item["hookAliveInterval"] = 0;
    item["rtpEnable"] = 1;
    item["status"] = server->IsConnected();
    item["createTime"] = "000";
    item["updateTime"] = "000";
    item["lastKeepaliveTime"] = "000";

    value["data"] = item;

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void MediaServerApi::check(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "ip", "port" });

    string port = parser._body["port"];
    std::string serverId = parser._body["ip"].get<std::string>() + ":" + port;

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto server = SMSServer::instance()->getServer(parser._body["serverId"]);
    if (!server) {
        value["code"] = 404;
        value["msg"] = "server not found";
        rsp.setContent(value.dump());
        rspFunc(rsp);

        return ;
    }
    json item;
    auto info = server->getInfo();
    item["id"] = info->ServerId;
    item["ip"] = info->IP;
    item["port"] = info->Port;
    item["hookIp"] = "hookIp";
    item["sdpIp"] = info->IP;
    item["streamIp"] = info->IP;
    item["httpPort"] = info->HttpServerPort;
    item["jt1078Port"] = info->JT1078Port;
    item["rtmpPort"] = info->RtmpPort;
    item["rtpProxyPort"] = info->RtpPort;
    item["rtspPort"] = info->RtspPort;
    item["hookAliveInterval"] = 0;
    item["rtpEnable"] = 1;
    item["status"] = server->IsConnected();
    item["createTime"] = "000";
    item["updateTime"] = "000";
    item["lastKeepaliveTime"] = "000";

    value["data"] = item;

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void MediaServerApi::save(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void MediaServerApi::del(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void MediaServerApi::load(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto serverList = SMSServer::instance()->getServerList();
    for (auto server : serverList) {
        json item;
        auto info = server.second->getInfo();
        item["id"] = info->ServerId;
        item["push"] = 0;
        item["proxy"] = 0;
        item["gbReceive"] = 0;
        item["gbSend"] = 0;

        value["data"].push_back(item);
    }

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void MediaServerApi::get(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto server = SMSServer::instance()->getServerByCircly();
    if (!server) {
        value["code"] = 404;
        value["msg"] = "server not found";
        rsp.setContent(value.dump());
        rspFunc(rsp);

        return ;
    }
    json item;
    auto info = server->getInfo();
    item["id"] = info->ServerId;
    item["ip"] = info->IP;
    item["port"] = info->Port;
    item["hookIp"] = "hookIp";
    item["sdpIp"] = info->IP;
    item["streamIp"] = info->IP;
    item["httpPort"] = info->HttpServerPort;
    item["jt1078Port"] = info->JT1078Port;
    item["rtmpPort"] = info->RtmpPort;
    item["rtpProxyPort"] = info->RtpPort;
    item["rtspPort"] = info->RtspPort;
    item["hookAliveInterval"] = 0;
    item["rtpEnable"] = 1;
    item["status"] = server->IsConnected();
    item["createTime"] = "000";
    item["updateTime"] = "000";
    item["lastKeepaliveTime"] = "000";

    value["data"] = item;

    value["code"] = 0;
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}