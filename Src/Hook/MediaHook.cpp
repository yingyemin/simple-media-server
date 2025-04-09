#include <string>

#include "MediaHook.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Http/HttpClientApi.h"
#include "Util/String.h"

using namespace std;

MediaHook::Ptr MediaHook::instance() 
{ 
    static MediaHook::Ptr hook(new MediaHook()); 
    return hook; 
}

void MediaHook::init()
{
    weak_ptr<MediaHook> wSelf = dynamic_pointer_cast<MediaHook>(shared_from_this());
    _type = Config::instance()->getAndListen([wSelf](const json& config){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        self->_type = Config::instance()->get("Hook", "Type");
        logInfo << "Hook type: " << self->_type;
    }, "Hook", "Type");
    _enableHook = Config::instance()->getAndListen([wSelf](const json& config){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        self->_enableHook = Config::instance()->get("Hook", "EnableHook");
        logInfo << "Hook type: " << self->_enableHook;
    }, "Hook", "EnableHook");

    HookManager::instance()->addHook(MEDIA_HOOK, shared_from_this());
}

void MediaHook::onStreamStatus(const StreamStatusInfo& info)
{
    json value;
    value["isOrigin"] = info.isOrigin;
    value["protocol"] = info.protocol;
    value["errorCode"] = info.errorCode;
    value["status"] = info.status;
    value["type"] = info.type;
    value["uri"] = info.uri;
    value["vhost"] = info.vhost;

    // static string type = Config::instance()->getAndListen([](const json& config){
    //     type = Config::instance()->get("Hook", "Type");
    //     logInfo << "Hook type: " << type;
    // }, "Hook", "Type");

    if (_type == "http") {
        static string url = Config::instance()->getAndListen([](const json& config){
            url = Config::instance()->get("Hook", "Http", "onStreamStatus");
            logInfo << "Hook url: " << url;
        }, "Hook", "Http", "onStreamStatus");

        HookManager::instance()->reportByHttp(url, "POST", value.dump());
    }
}

void MediaHook::onStreamHeartbeat(const StreamHeartbeatInfo& info)
{
    json value;
    value["protocol"] = info.protocol;
    value["type"] = info.type;
    value["uri"] = info.uri;
    value["vhost"] = info.vhost;
    value["playerCount"] = info.playerCount;
    value["bitrate"] = info.bitrate;
    value["bytes"] = info.bytes;
    value["createTime"] = info.createTime;
    value["currentTime"] = info.currentTime;

    // static string type = Config::instance()->getAndListen([](const json& config){
    //     type = Config::instance()->get("Hook", "Type");
    //     logInfo << "Hook type: " << type;
    // }, "Hook", "Type");

    if (_type == "http") {
        static string url = Config::instance()->getAndListen([](const json& config){
            url = Config::instance()->get("Hook", "Http", "onStreamHeartbeat");
            logInfo << "Hook url: " << url;
        }, "Hook", "Http", "onStreamHeartbeat");

        HookManager::instance()->reportByHttp(url, "POST", value.dump());
    }
}

// 使用类作为参数，好处是以后修改参数没那么麻烦，坏处是有地方漏了，编译的时候也发现不了
// 本项目使用类主要是图方便，谨慎用之
void MediaHook::onPublish(const PublishInfo& info, const function<void(const PublishResponse& rsp)>& cb)
{
    if (!_enableHook) {
        PublishResponse rsp;
        rsp.authResult = true;

        cb(rsp);
        return ;
    }

    json value;
    value["protocol"] = info.protocol;
    value["type"] = info.type;
    value["uri"] = info.uri;
    value["vhost"] = info.vhost;
    value["params"] = info.params;

    // static string type = Config::instance()->getAndListen([](const json& config){
    //     type = Config::instance()->get("Hook", "Type");
    //     logInfo << "Hook type: " << type;
    // }, "Hook", "Type");

    if (_type == "http") {
        static string url = Config::instance()->getAndListen([](const json& config){
            url = Config::instance()->get("Hook", "Http", "onPublish");
            logInfo << "Hook url: " << url;
        }, "Hook", "Http", "onPublish");

        HookManager::instance()->reportByHttp(url, "POST", value.dump(), [cb, info](const string& err, const nlohmann::json& res){
            logTrace << "on publish: " << url;
            PublishResponse rsp;
            if (!err.empty()) {
                rsp.authResult = false;
                rsp.err = err;
                cb(rsp);

                return ;
            }

            rsp.appName = res.value("appName", "");
            rsp.streamName = res.value("streamName", "");

            if (res.find("authResult") == res.end()) {
                rsp.authResult = false;
                rsp.err = "authResult is empty";
                cb(rsp);

                logInfo << "on publish failed, authResult is empty, path: " << info.uri;

                return ;
            }
            
            rsp.authResult = res["authResult"];
            cb(rsp);
        });
    }
}

void MediaHook::onPlay(const PlayInfo& info, const function<void(const PlayResponse& rsp)>& cb)
{
    if (!_enableHook) {
        PlayResponse rsp;
        rsp.authResult = true;

        cb(rsp);
        return ;
    }

    json value;
    value["protocol"] = info.protocol;
    value["type"] = info.type;
    value["uri"] = info.uri;
    value["vhost"] = info.vhost;
    value["params"] = info.params;

    // static string type = Config::instance()->getAndListen([](const json& config){
    //     type = Config::instance()->get("Hook", "Type");
    //     logInfo << "Hook type: " << type;
    // }, "Hook", "Type");

    if (_type == "http") {
        static string url = Config::instance()->getAndListen([](const json& config){
            url = Config::instance()->get("Hook", "Http", "onPlay");
            logInfo << "Hook url: " << url;
        }, "Hook", "Http", "onPlay");

        HookManager::instance()->reportByHttp(url, "POST", value.dump(), [cb, info](const string& err, const nlohmann::json& res){
            PlayResponse rsp;
            if (!err.empty()) {
                rsp.authResult = false;
                rsp.err = err;
                cb(rsp);

                return ;
            }

            if (res.find("authResult") == res.end()) {
                rsp.authResult = false;
                rsp.err = "authResult is empty";
                cb(rsp);

                logInfo << "on play failed, authResult is empty, path: " << info.uri;

                return ;
            }
            
            rsp.authResult = res["authResult"];
            cb(rsp);
        });
    }
}

void MediaHook::onPlayer(const PlayerInfo& info)
{
    json value;
    value["protocol"] = info.protocol;
    value["type"] = info.type;
    value["uri"] = info.uri;
    value["vhost"] = info.vhost;
    value["ip"] = info.ip;
    value["status"] = info.status;
    value["port"] = info.port;

    // static string type = Config::instance()->getAndListen([](const json& config){
    //     type = Config::instance()->get("Hook", "Type");
    //     logInfo << "Hook type: " << type;
    // }, "Hook", "Type");

    if (_type == "http") {
        static string url = Config::instance()->getAndListen([](const json& config){
            url = Config::instance()->get("Hook", "Http", "onPlayer");
            logInfo << "Hook url: " << url;
        }, "Hook", "Http", "onPlayer");

        HookManager::instance()->reportByHttp(url, "POST", value.dump(), [](const string& err, const nlohmann::json& res){
            
        });
    }
}

void MediaHook::onNonePlayer(const string& protocol, const string& uri, 
            const string& vhost, const string& type)
{
    json value;
    value["protocol"] = protocol;
    value["type"] = type;
    value["uri"] = uri;
    value["vhost"] = vhost;

    if (_type == "http") {
        static string url = Config::instance()->getAndListen([](const json& config){
            url = Config::instance()->get("Hook", "Http", "onNonePlayer");
            logInfo << "Hook url: " << url;
        }, "Hook", "Http", "onNonePlayer");

        HookManager::instance()->reportByHttp(url, "POST", value.dump());
    }
}

void MediaHook::onKeepAlive(const ServerInfo& info)
{
    json value;
    value["serverId"] = info.ip + ":" + to_string(info.port);
    value["ip"] = info.ip;
    value["port"] = info.port;
    value["httpServerPort"] = info.httpServerPort;
    value["rtmpServerPort"] = info.rtmpServerPort;
    value["rtspServerPort"] = info.rtspServerPort;
    value["originCount"] = info.originCount;
    value["playerCount"] = info.playerCount;
    value["memUsage"] = info.memUsage;

    logInfo << "server info: " << value.dump();

    if (_type == "http") {
        static string url = Config::instance()->getAndListen([](const json& config){
            url = Config::instance()->get("Hook", "Http", "onKeepAlive");
            logInfo << "Hook url: " << url;
        }, "Hook", "Http", "onKeepAlive");

        HookManager::instance()->reportByHttp(url, "POST", value.dump());
    }
}

void MediaHook::onRegisterServer(const RegisterServerInfo& info)
{
    json value;
    value["serverId"] = info.ip + ":" + to_string(info.port);
    value["ip"] = info.ip;
    value["port"] = info.port;
    value["httpServerPort"] = info.httpServerPort;
    value["rtmpServerPort"] = info.rtmpServerPort;
    value["rtspServerPort"] = info.rtspServerPort;

    logInfo << "server info: " << value.dump();

    if (_type == "http") {
        static string url = Config::instance()->getAndListen([](const json& config){
            url = Config::instance()->get("Hook", "Http", "onRegisterServer");
            logInfo << "Hook url: " << url;
        }, "Hook", "Http", "onRegisterServer");

        HookManager::instance()->reportByHttp(url, "POST", value.dump());
    }
}