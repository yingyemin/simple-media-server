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

// using namespace std;

class MediaHook : public HookBase
{
public:
    using Ptr = std::shared_ptr<MediaHook>;

    MediaHook() {}
    ~MediaHook() {}

public:
    static MediaHook::Ptr instance();
    void init();

    void onStreamStatus(const StreamStatusInfo& info) override;
    void onStreamHeartbeat(const StreamHeartbeatInfo& info) override;
    void onPublish(const PublishInfo& info, const std::function<void(const PublishResponse& rsp)>& cb) override;
    void onPlay(const PlayInfo& info, const std::function<void(const PlayResponse& rsp)>& cb) override;
    void onPlayer(const PlayerInfo& info) override;
    void onNonePlayer(const std::string& protocol, const std::string& uri, 
                        const std::string& vhost, const std::string& type) override;

    void onKeepAlive(const ServerInfo& info) override;
    void onRegisterServer(const RegisterServerInfo& info) override;
    void onRecord(const OnRecordInfo& info) override;
    void onStreamNotFound(const OnStreamNotFoundInfo& info, 
                    const std::function<void(const OnStreamNotFoundResponse& rsp)>& cb) override;

private:
    bool _enableHook = true;
    std::string _type = "http";
};


#endif //MediaHook_H
