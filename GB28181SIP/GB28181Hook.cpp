#include <string>

#include "GB28181Hook.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Http/HttpClientApi.h"
#include "Util/String.hpp"

using namespace std;

GB28181Hook::Ptr GB28181Hook::instance() 
{ 
    static GB28181Hook::Ptr hook(new GB28181Hook()); 
    return hook; 
}

void GB28181Hook::init()
{
    weak_ptr<GB28181Hook> wSelf = dynamic_pointer_cast<GB28181Hook>(shared_from_this());
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

    // HookManager::instance()->addHook(MEDIA_HOOK, shared_from_this());
}

void GB28181Hook::onDeviceStatus(const DeviceStatusInfo& info)
{
    json value;
    value["deviceId"] = info.deviceId;
    value["serverId"] = info.serverId;
    value["errorCode"] = info.errorCode;
    value["status"] = info.status;

    static string ip = Config::instance()->getAndListen([](const json &config){
        ip = Config::instance()->get("LocalIp");
    }, "LocalIp");

    static int httpPort = Config::instance()->getAndListen([](const json& config){
        httpPort = Config::instance()->get("Http", "Api", "Api1", "port");
    }, "Http", "Api", "Api1", "port");

    value["serverId"] = ip + ":" + to_string(httpPort);

    if (_type == "http") {
        static string url = Config::instance()->getAndListen([](const json& config){
            url = Config::instance()->get("Hook", "Http", "onDeviceStatus");
            logInfo << "Hook url: " << url;
        }, "Hook", "Http", "onDeviceStatus");

        HookManager::instance()->reportByHttp(url, "POST", value.dump());
    }
}

void GB28181Hook::onDeviceHeartbeat(const DeviceHeartbeatInfo& info)
{
    json value;
    
    value["deviceId"] = info.deviceId;
    value["serverId"] = info.serverId;
    value["errorCode"] = info.errorCode;
    value["timestamp"] = info.timestamp;

    // static string type = Config::instance()->getAndListen([](const json& config){
    //     type = Config::instance()->get("Hook", "Type");
    //     logInfo << "Hook type: " << type;
    // }, "Hook", "Type");

    if (_type == "http") {
        static string url = Config::instance()->getAndListen([](const json& config){
            url = Config::instance()->get("Hook", "Http", "onDeviceHeartbeat");
            logInfo << "Hook url: " << url;
        }, "Hook", "Http", "onDeviceHeartbeat");

        HookManager::instance()->reportByHttp(url, "POST", value.dump());
    }
}

void GB28181Hook::onDeviceInfo(const DeviceInfoInfo& info)
{
    json value;
    
    value["deviceId"] = info.deviceId;
    value["serverId"] = info.serverId;
    value["errorCode"] = info.errorCode;
    value["manufacturer"] = info.manufacturer;
    value["name"] = info.name;
    value["channelCount"] = info.channelCount;

    // static string type = Config::instance()->getAndListen([](const json& config){
    //     type = Config::instance()->get("Hook", "Type");
    //     logInfo << "Hook type: " << type;
    // }, "Hook", "Type");

    if (_type == "http") {
        static string url = Config::instance()->getAndListen([](const json& config){
            url = Config::instance()->get("Hook", "Http", "onDeviceInfo");
            logInfo << "Hook url: " << url;
        }, "Hook", "Http", "onDeviceInfo");

        HookManager::instance()->reportByHttp(url, "POST", value.dump());
    }
}

void GB28181Hook::onDeviceChannel(const std::string& deviceId,  const std::string& serverId, 
                            const DeviceChannelInfo& info)
{
    json value;
    
    value["deviceId"] = deviceId;
    value["serverId"] = serverId;
    value["channelId"] = info.channelId;
    value["name"] = info.name;
    value["paraentId"] = info.paraentId;

    // static string type = Config::instance()->getAndListen([](const json& config){
    //     type = Config::instance()->get("Hook", "Type");
    //     logInfo << "Hook type: " << type;
    // }, "Hook", "Type");

    if (_type == "http") {
        static string url = Config::instance()->getAndListen([](const json& config){
            url = Config::instance()->get("Hook", "Http", "onDeviceChannel");
            logInfo << "Hook url: " << url;
        }, "Hook", "Http", "onDeviceChannel");

        HookManager::instance()->reportByHttp(url, "POST", value.dump());
    }
}

void GB28181Hook::onKeepAlive(const ServerInfo& info)
{
    json value;
    value["serverId"] = info.ip + ":" + to_string(info.port);
    value["ip"] = info.ip;
    value["port"] = info.port;
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

void GB28181Hook::onRegisterServer(const RegisterServerInfo& info)
{
    json value;
    value["serverId"] = info.ip + ":" + to_string(info.port);
    value["ip"] = info.ip;
    value["port"] = info.port;

    logInfo << "server info: " << value.dump();

    if (_type == "http") {
        static string url = Config::instance()->getAndListen([](const json& config){
            url = Config::instance()->get("Hook", "Http", "onRegisterServer");
            logInfo << "Hook url: " << url;
        }, "Hook", "Http", "onRegisterServer");

        HookManager::instance()->reportByHttp(url, "POST", value.dump());
    }
}

