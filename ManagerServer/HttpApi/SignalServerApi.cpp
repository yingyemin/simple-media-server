#include "SignalServerApi.h"
#include "Logger.h"
#include "Common/Config.h"
#include "../Common/ApiUtil.h"
#include "Manager/SipServer.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void SignalServerApi::initApi()
{
    g_mapApi.emplace("/api/v1/signal/server/onRegisterServer", SignalServerApi::onRegisterServer);
    g_mapApi.emplace("/api/v1/signal/server/onKeepAlive", SignalServerApi::onKeepAlive);
    g_mapApi.emplace("/api/v1/signal/server/list", SignalServerApi::list);
    g_mapApi.emplace("/api/v1/signal/server/query", SignalServerApi::query);
    g_mapApi.emplace("/api/v1/signal/server/get", SignalServerApi::get);
}

void SignalServerApi::onRegisterServer(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "serverId" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto server = SipServer::instance()->getServer(parser._body["serverId"]);
    if (!server) {
        server = make_shared<SipServer>();
        auto info = make_shared<SipServerInfo>();
        server->setInfo(info);
        SipServer::instance()->addServer(parser._body["serverId"], server);
    }
    auto serverInfo = server->getInfo();
    //流媒体服务 IP
	serverInfo->IP = parser._body.value("ip", "");
	//流媒体服务 端口
	serverInfo->Port = getInt(parser._body, "port", 8010);
    serverInfo->ServerId = parser._body["serverId"];
	auto base_url = "http://" + serverInfo->IP + ":" + to_string(serverInfo->Port);
    server->setBaseUrl(base_url);
    server->UpdateHeartbeatTime();

    value["code"] = "200";
    value["msg"] = "success";
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void SignalServerApi::onKeepAlive(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "serverId" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto server = SipServer::instance()->getServer(parser._body["serverId"]);
    if (!server) {
        server = make_shared<SipServer>();
        auto info = make_shared<SipServerInfo>();
        server->setInfo(info);
        SipServer::instance()->addServer(parser._body["serverId"], server);
    }
    auto serverInfo = server->getInfo();
	//流媒体服务 端口
	serverInfo->Port = getInt(parser._body, "port", 8010);
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

void SignalServerApi::list(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto serverList = SipServer::instance()->getServerList();
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
        item["hookAliveInterval"] = 0;
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

void SignalServerApi::query(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    checkArgs(parser._body, { "serverId" });

    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto server = SipServer::instance()->getServer(parser._body["serverId"]);
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
    item["hookAliveInterval"] = 0;
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

void SignalServerApi::get(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    HttpResponse rsp;
    rsp._status = 200;
    json value;

    auto server = SipServer::instance()->getServerByCircly();
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
    item["hookAliveInterval"] = 0;
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