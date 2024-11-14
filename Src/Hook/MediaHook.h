#ifndef MediaHook_H
#define MediaHook_H

#include <mutex>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include "Net/Buffer.h"
#include "Common/json.hpp"

using namespace std;

// 流上下线的参数
class StreamStatusInfo
{
public:
    string protocol;
    string uri;
    string vhost;
    string type;
    string status;
    string errorCode;
};

// 鉴权请求参数
class PublishInfo
{
public:
    string protocol;
    string uri;
    string vhost;
    string type;
    string params;
};

// 鉴权的返回参数
class PublishResponse
{
public:
    bool authResult = false;
    string err;
};

// 鉴权请求参数
class PlayInfo
{
public:
    string protocol;
    string uri;
    string vhost;
    string type;
    string params;
};

// 鉴权的返回参数
class PlayResponse
{
public:
    bool authResult = false;
    string err;
};

class PlayerInfo
{
public:
    string protocol;
    string uri;
    string vhost;
    string type;
    string ip;
    string status;
    int port = 0;
};

class MediaHook : public enable_shared_from_this<MediaHook>
{
public:
    using Ptr = shared_ptr<MediaHook>;

    MediaHook() {}
    ~MediaHook() {}

public:
    static MediaHook::Ptr instance();
    void init();
    void reportByHttp(const string& url, const string&method, const string& msg, const function<void(const string& err, 
                const nlohmann::json& res)>& cb = [](const string& err, const nlohmann::json& res){});
    void onStreamStatus(const StreamStatusInfo& info);
    void onPublish(const PublishInfo& info, const function<void(const PublishResponse& rsp)>& cb);
    void onPlay(const PlayInfo& info, const function<void(const PlayResponse& rsp)>& cb);
    void onPlayer(const PlayerInfo& info);
    void onNonePlayer(const string& protocol, const string& uri, 
                        const string& vhost, const string& type);

private:
    bool _enableHook = true;
    string _type = "http";
};


#endif //MediaHook_H
