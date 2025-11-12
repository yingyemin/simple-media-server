#pragma once
#include <unordered_map>
#include <mutex>
#include <memory>
#include <string>
#include <functional>

struct SipServerInfo {
	std::string ServerId;
	std::string IP;
	int Port;
};

class SipServer : public std::enable_shared_from_this<SipServer>
{
public:
	using Ptr = std::shared_ptr<SipServer>;

	static std::shared_ptr<SipServer> instance();

	void Init();

	void addServer(const std::string& serverId, const SipServer::Ptr& server);
	void removeServer(const std::string& serverId);
	SipServer::Ptr getServer(const std::string& serverId);
	SipServer::Ptr getServerByCircly();
	std::unordered_map<std::string, SipServer::Ptr> getServerList();

	/////////////////////////////////////////////////////////////////////////////////////////////////

	SipServer() = default;

	void UpdateHeartbeatTime(time_t t = time(nullptr));
	void UpdateStatus(bool flag = true);
	bool IsConnected();

	std::string getIP() {return _info->IP;}
	int getPort() {return _info->Port;}
	std::shared_ptr<SipServerInfo> getInfo() {return _info;}
	void setInfo(std::shared_ptr<SipServerInfo> info) {_info = info;}
	void setBaseUrl(const std::string& url) {_base_url = url;}

	void invite(const std::string& deviceId, const std::string& channelId, int channelNum, 
					const std::string& ip, int port, const std::string& ssrc);

private:
	uint16_t _index = 0;
	std::shared_ptr<SipServerInfo> _info = nullptr;
	time_t _last_heartbeat_time = 0;
	bool _connected = false;
	std::mutex _mutex;
	std::string _base_url ;
	std::unordered_map<std::string, SipServer::Ptr> _server_map;
};