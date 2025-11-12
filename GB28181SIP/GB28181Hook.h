#ifndef GB28181Hook_H
#define GB28181Hook_H

#include <mutex>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include "Net/Buffer.h"
#include "Common/json.hpp"
#include "Common/HookManager.h"

// using namespace std;

class DeviceStatusInfo
{
public:
    std::string serverId;
    std::string deviceId;
    std::string status;
    std::string errorCode;
};

class DeviceHeartbeatInfo
{
public:
    uint64_t timestamp;
    std::string serverId;
    std::string deviceId;
    std::string errorCode;
};

class DeviceInfoInfo
{
public:
    int channelCount;
    std::string serverId;
    std::string deviceId;
    std::string name;
    std::string manufacturer;
    std::string errorCode;
};

class DeviceChannelInfo
{
public:
    std::string channelId;
    std::string paraentId;
    std::string name;
    std::string errorCode;
};

class GB28181Hook : public std::enable_shared_from_this<GB28181Hook>
{
public:
    using Ptr = std::shared_ptr<GB28181Hook>;

    GB28181Hook() {}
    ~GB28181Hook() {}

public:
    static GB28181Hook::Ptr instance();
    void init();

    void onDeviceStatus(const DeviceStatusInfo& info);
    void onDeviceHeartbeat(const DeviceHeartbeatInfo& info);
    void onDeviceInfo(const DeviceInfoInfo& info);
    void onDeviceChannel(const std::string& deviceId,  const std::string& serverId, 
                            const DeviceChannelInfo& info);

    void onKeepAlive(const ServerInfo& info);
    void onRegisterServer(const RegisterServerInfo& info);

private:
    bool _enableHook = true;
    std::string _type = "http";
};


#endif //GB28181Hook_H
