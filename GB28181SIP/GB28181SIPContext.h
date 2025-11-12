#ifndef GB28181SIPContext_H
#define GB28181SIPContext_H

#include "Buffer.h"
#include "Util/TimeClock.h"
#include "EventPoller/EventLoop.h"
#include "Net/Socket.h"
#include "SipMessage.h"
#include "GB28181Channel.h"
#include "pugixml.hpp"

#include <memory>
#include <unordered_map>

// using namespace std;

class MediaInfo{
public:
    std::string channelId;
    int channelNum;
    std::string ip;
    int port;
    uint32_t ssrc;
};

class DeviceInfo
{
public:
	using Ptr = std::shared_ptr<DeviceInfo>;

	DeviceInfo() = default;

	std::string _device_id;
	std::string _name;
	std::string _nickname;
	std::string _ip;
	std::string _port;
	std::string _transport = "UDP";
	std::string _manufacturer;
	std::string _model;
	int _status = 0;

	std::string _stream_ip;//收流IP

	time_t _regist_time = 0;
	time_t _last_time = 0;

	int _channel_count = 0;
	std::string _parent_id;

	bool _registered = false;
};

class GB28181SIPContext : public std::enable_shared_from_this<GB28181SIPContext>
{
public:
    using Ptr = std::shared_ptr<GB28181SIPContext>;
    using Wptr = std::weak_ptr<GB28181SIPContext>;

    GB28181SIPContext(const EventLoop::Ptr& loop, const std::string& deviceId, const std::string& vhost, const std::string& protocol, const std::string& type);
    ~GB28181SIPContext();
public:
    bool init();
    void onSipPacket(const Socket::Ptr& socket, const SipRequest::Ptr& req, struct sockaddr* addr = nullptr, int len = 0, bool sort = false);
    void heartbeat();
    bool isAlive() {return _alive;}
    void catalog();
    void deviceInfo();
    void invite(const MediaInfo& mediainfo);
    void bye(const std::string& channelId, const std::string& callId);
    void bye(const MediaInfo& mediainfo);
    void sendMessage(const char* msg, size_t size);
    DeviceInfo::Ptr getDeviceInfo() { return _deviceInfo; }
    std::unordered_map<std::string, Channel::Ptr> getChannelList();
    std::unordered_map<std::string, std::unordered_map<std::string, MediaInfo>> getMediaInfoList();

private:
    void handleCatalog(const pugi::xml_document& doc);
    void handleNotifyCatalog(const pugi::xml_document& doc);
    void handleDeviceInfo(const pugi::xml_document& doc);

    void handleNotify(const SipRequest::Ptr& req, std::stringstream& ss);
    void handleRegister(const SipRequest::Ptr& req, std::stringstream& ss);
    void handleMessage(const SipRequest::Ptr& req, std::stringstream& ss);
    void handleInvite(const SipRequest::Ptr& req, std::stringstream& ss);

private:
    bool _alive = true;
    bool _catalogFinish = true;
    uint64_t _catalogSn = 0;
    uint64_t _notifyCatalogSn = 0;
    std::string _serverId;
    std::string _deviceId;
    std::string _vhost;
    std::string _protocol;
    std::string _type;
    std::string _payloadType = "ps";

    TimeClock _timeClock;
    TimeClock _catalogTime;
    TimeClock _keepAliveTime;

    SipStack _sipStack;
    std::shared_ptr<sockaddr> _addr;
    std::shared_ptr<SipRequest> _req = NULL;
    Socket::Ptr _socket;
    EventLoop::Ptr _loop;

    DeviceInfo::Ptr _deviceInfo;

    std::mutex _mtxChannel;
    std::unordered_map<std::string, Channel::Ptr> _mapChannel;

    std::mutex _mtx;
    // channelId, callId, mediainfo
    std::unordered_map<std::string, std::unordered_map<std::string, MediaInfo>> _mapMediaInfo;
};



#endif //GB28181SIPContext_H
