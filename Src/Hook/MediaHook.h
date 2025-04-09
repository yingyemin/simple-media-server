#ifndef MediaHook_H
#define MediaHook_H

#include <mutex>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include "Net/Buffer.h"
#include "Common/json.hpp"
#include "Common/HookManager.h"

using namespace std;

class MediaHook : public HookBase
{
public:
    using Ptr = shared_ptr<MediaHook>;

    MediaHook() {}
    ~MediaHook() {}

public:
    static MediaHook::Ptr instance();
    void init();

    void onStreamStatus(const StreamStatusInfo& info) override;
    void onStreamHeartbeat(const StreamHeartbeatInfo& info) override;
    void onPublish(const PublishInfo& info, const function<void(const PublishResponse& rsp)>& cb) override;
    void onPlay(const PlayInfo& info, const function<void(const PlayResponse& rsp)>& cb) override;
    void onPlayer(const PlayerInfo& info) override;
    void onNonePlayer(const string& protocol, const string& uri, 
                        const string& vhost, const string& type) override;

    void onKeepAlive(const ServerInfo& info) override;
    void onRegisterServer(const RegisterServerInfo& info) override;

private:
    bool _enableHook = true;
    string _type = "http";
};


#endif //MediaHook_H
