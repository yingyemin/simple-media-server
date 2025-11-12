#pragma once
#include <unordered_map>
#include <mutex>
#include <memory>
#include <string>
#include <functional>

struct MediaServerInfo {
	std::string ServerId;
	std::string IP;
	int Port;
	int RtpPort;
	int PlayWait;
	int RtmpPort;
	int RtspPort;
	int HttpServerPort;
	int JT1078Port;
	std::string Secret;
	bool SinglePortMode = false;
};

class SMSServer : public std::enable_shared_from_this<SMSServer>
{
public:
	using Ptr = std::shared_ptr<SMSServer>;

	static std::shared_ptr<SMSServer> instance();

	void Init();

	void addServer(const std::string& serverId, const SMSServer::Ptr& server);
	void removeServer(const std::string& serverId);
	SMSServer::Ptr getServer(const std::string& serverId);
	SMSServer::Ptr getServerByCircly();
	std::unordered_map<std::string, SMSServer::Ptr> getServerList();

	/////////////////////////////////////////////////////////////////////////////////////////////////

	SMSServer() = default;

	void OpenRtpServer(const std::string& str_ssrc, const std::string& streamId,int socket_type,int active=0, const std::function<void(int port)>& cb = nullptr);
	void CloseRtpServer(const std::string& stream_id);

	void OpenRtpSender(const std::string& str_ssrc,const std::string& ip,int port,int socket_type,int active=1,const std::string& stream_id="",
		const std::function<void(int port)>& cb = nullptr);
	void StartRtpSender(const std::string& str_ssrc,const std::string& ip,int port,
									const std::function<void()>& cb = nullptr);
	std::string ListRtpServer();

	void CloseRtpSender(const std::string& stream_id);
	bool SinglePortMode();
	int FixedRtpPort();
	int MaxPlayWaitTime();

	void UpdateHeartbeatTime(time_t t = time(nullptr));
	void UpdateStatus(bool flag = true);
	bool IsConnected();
	std::string getWebrtcPublishApi();
	std::string getWebrtcPlayApi();

	std::string getIP() {return _info->IP;}
	int getPort() {return _info->Port;}
	std::shared_ptr<MediaServerInfo> getInfo() {return _info;}
	void setInfo(std::shared_ptr<MediaServerInfo> info) {_info = info;}
	void setBaseUrl(const std::string& url) {_base_url = url;}

private:
	uint16_t _index = 0;
	std::shared_ptr<MediaServerInfo> _info = nullptr;
	time_t _last_heartbeat_time = 0;
	bool _connected = false;
	std::mutex _mutex;
	std::string _base_url ;
	std::unordered_map<std::string, SMSServer::Ptr> _server_map;
};