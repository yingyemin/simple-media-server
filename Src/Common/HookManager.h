#ifndef HookManager_H
#define HookManager_H

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>

#include "Common/json.hpp"

#define MEDIA_HOOK "MediaHook"

// 流上下线的参数
class StreamStatusInfo
{
public:
    StreamStatusInfo(bool isOrigin, const std::string& protocol, const std::string& uri, const std::string& vhost, 
        const std::string& type, const std::string& status, const std::string& action, const std::string& errorCode)
        :isOrigin(isOrigin), protocol(protocol), uri(uri), vhost(vhost)
        , type(type), status(status), action(action), errorCode(errorCode)
    {
    }

public:
    bool isOrigin = false;
    std::string protocol;
    std::string uri;
    std::string vhost;
    std::string type;
    std::string status;
    std::string action;
    std::string errorCode;
};

// 流的心跳
class StreamHeartbeatInfo
{
public:
    int playerCount = 0;
    uint64_t bytes = 0;
    uint64_t createTime = 0;
    uint64_t currentTime = 0;
    float bitrate = 0;
    std::string protocol;
    std::string uri;
    std::string vhost;
    std::string type;
};

// 鉴权请求参数
class PublishInfo
{
public:
    std::string protocol;
    std::string uri;
    std::string vhost;
    std::string type;
    std::string params;
};

// 鉴权的返回参数
class PublishResponse
{
public:
    bool authResult = false;
    // 用于1078或者28181重新命名
    std::string appName;
    std::string streamName;
    std::string err;
};

// 鉴权请求参数
class PlayInfo
{
public:
    std::string protocol;
    std::string uri;
    std::string vhost;
    std::string type;
    std::string params;
};

// 鉴权的返回参数
class PlayResponse
{
public:
    bool authResult = false;
    std::string err;
};

class PlayerInfo
{
public:
    std::string protocol;
    std::string uri;
    std::string vhost;
    std::string type;
    std::string ip;
    std::string status;
    int port = 0;
};

class ServerInfo
{
public:
    std::string ip;
    int port = 0;
    uint16_t httpServerPort = 0;
    uint16_t rtspServerPort = 0;
    uint16_t rtmpServerPort = 0;
    uint64_t originCount = 0;
    uint64_t playerCount = 0;
    float memUsage = 0;
};

class RegisterServerInfo
{
public:
    std::string ip;
    int port = 0;
    uint16_t httpServerPort = 0;
    uint16_t rtspServerPort = 0;
    uint16_t rtmpServerPort = 0;
    uint16_t jt1078ServerPort = 0;
};

class OnRecordInfo
{
public:
    uint64_t startTime = 0;
    uint64_t endTime = 0;
    uint64_t fileSize = 0;
    uint64_t duration = 0;
    std::string filePath;
    std::string fileName;
    std::string uri;
    std::string status;
};

class OnStreamNotFoundInfo
{
public:
    std::string uri;
};

class OnStreamNotFoundResponse
{
public:
    std::string uri;
    std::string pullUrl;
};

class HookBase : public std::enable_shared_from_this<HookBase>
{
public:
    using Ptr = std::shared_ptr<HookBase>;

    virtual void onStreamStatus(const StreamStatusInfo& info) = 0;
    virtual void onStreamHeartbeat(const StreamHeartbeatInfo& info) = 0;
    virtual void onPublish(const PublishInfo& info, const std::function<void(const PublishResponse& rsp)>& cb) = 0;
    virtual void onPlay(const PlayInfo& info, const std::function<void(const PlayResponse& rsp)>& cb) = 0;
    virtual void onPlayer(const PlayerInfo& info) = 0;
    virtual void onNonePlayer(const std::string& protocol, const std::string& uri, 
                        const std::string& vhost, const std::string& type) = 0;

    virtual void onKeepAlive(const ServerInfo& info) = 0;
    virtual void onRegisterServer(const RegisterServerInfo& info) = 0;
    virtual void onRecord(const OnRecordInfo& info) = 0;
    virtual void onStreamNotFound(const OnStreamNotFoundInfo& info, const std::function<void(const OnStreamNotFoundResponse& rsp)>& cb) = 0;
};

class HookManager : public std::enable_shared_from_this<HookManager>
{
public: 
    using Ptr = std::shared_ptr<HookManager>;
    using hookReportFunc = std::function<void(const std::string& url, const std::string& method, const std::string& msg, const std::function<void(const std::string& err, 
                const nlohmann::json& res)>& cb)>;

    static HookManager::Ptr instance();
    void addHook(const std::string& hookName, const HookBase::Ptr& hook);
    HookBase::Ptr getHook(const std::string& hookName);
    void setOnHookReportByHttp(const hookReportFunc& func);
    void reportByHttp(const std::string& url, const std::string&method, const std::string& msg, const std::function<void(const std::string& err, 
                const nlohmann::json& res)>& cb = [](const std::string& err, const nlohmann::json& res){});

private:
    std::mutex _mutex;
    std::unordered_map<std::string, HookBase::Ptr> _mapHook;
    hookReportFunc _onHookReport;
};


#endif //HookManager_H
