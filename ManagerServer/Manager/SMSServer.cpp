#include "SMSServer.h"
#include "Http/HttpClientApi.h"
#include "Log/Logger.h"
#include "EventPoller/EventLoopPool.h"

#include "Common/Config.h"

using namespace std;

std::shared_ptr<SMSServer> SMSServer::instance()
{
	static std::shared_ptr<SMSServer> manager(new SMSServer());
	return manager;
}

void SMSServer::Init()
{
	// static std::string ip = Config::instance()->getAndListen([](const json& config) {
	// 	ip = Config::instance()->get("SipServer", "SMSServer", "ip");
	// 	}, "SipServer", "SMSServer", "ip");

	// static int port = Config::instance()->getAndListen([](const json& config) {
	// 	ip = Config::instance()->get("SipServer", "SMSServer", "port");
	// 	}, "SipServer", "SMSServer", "port");

	// static int RtmpPort = Config::instance()->getAndListen([](const json& config) {
	// 	ip = Config::instance()->get("SipServer", "SMSServer", "RtmpPort");
	// 	}, "SipServer", "SMSServer", "RtmpPort");

	// static int RtpPort = Config::instance()->getAndListen([](const json& config) {
	// 	ip = Config::instance()->get("SipServer", "SMSServer", "RtpPort");
	// 	}, "SipServer", "SMSServer", "RtpPort");

	// static int wait = Config::instance()->getAndListen([](const json& config) {
	// 	ip = Config::instance()->get("SipServer", "SMSServer", "PlayWait");
	// 	}, "SipServer", "SMSServer", "PlayWait");

	// auto info = std::make_shared<MediaServerInfo>();

	// //流媒体服务 IP
	// info->IP = ip;
	// //流媒体服务 端口
	// info->Port = port;
	// info->PlayWait = wait;
	// info->RtpPort = RtpPort;
	// info->RtmpPort = RtmpPort;
	// info->ServerId = info->IP + ":" + to_string(info->Port);

	// std::string base_url = "http://" + info->IP + ":" + to_string(info->Port);

	// auto server = make_shared<SMSServer>();
	// server->setBaseUrl(base_url);
	// server->setInfo(info);

	// addServer(info->ServerId, server);
}

void SMSServer::addServer(const std::string& serverId, const SMSServer::Ptr& server)
{
	{
		std::lock_guard<mutex> g(_mutex);
		_server_map[serverId] = server;
	}
}

void SMSServer::removeServer(const std::string& serverId)
{
	std::lock_guard<mutex> g(_mutex);
	_server_map.erase(serverId);
}

SMSServer::Ptr SMSServer::getServer(const std::string& serverId)
{
	std::lock_guard<mutex> g(_mutex);
	auto it = _server_map.find(serverId);
	if (it != _server_map.end())
		return it->second;
	return nullptr;
}

SMSServer::Ptr SMSServer::getServerByCircly()
{
	std::lock_guard<mutex> g(_mutex);
	int i = 0;
	SMSServer::Ptr lastOnline;
	for (auto it = _server_map.begin(); it != _server_map.end(); ++it)
	{
		if (!it->second->IsConnected()) {
			continue;
		}

		lastOnline = it->second;

		if (++i == _index) {
			_index = (++_index) % _server_map.size();
			return it->second;
		}
	}
	
	if (i > 0) {
		_index %= i;
	}

	return lastOnline;
}

std::unordered_map<std::string, SMSServer::Ptr> SMSServer::getServerList()
{
	std::lock_guard<mutex> g(_mutex);
	return _server_map;
}

/////////////////////////////////////////////////////////////////////////////

void SMSServer::UpdateHeartbeatTime(time_t t)
{
	// std::lock_guard<std::mutex> g(_mutex);
	// _last_heartbeat_time = t;

	// if (_delay_task)
	// 	_delay_task->cancel();

	// _delay_task = toolkit::EventPollerPool::Instance().getPoller()->doDelayTask(1000 * 10, [this]()
	// 	{
	// 		auto now = time(nullptr);
	// 		if (now - _last_heartbeat_time > 6)
	// 			UpdateStatus(false);
	// 		else
	// 			UpdateStatus(true);
	// 		return 0;
	// 	});

	if (_last_heartbeat_time == 0) {
		std::weak_ptr<SMSServer> wSelf = shared_from_this();
		EventLoopPool::instance()->getLoopByCircle()->addTimerTask(1000 * 10, [wSelf](){
			auto self = wSelf.lock();
			if (!self) {
				return 0;
			}

			auto now = time(nullptr);
			if (now - self->_last_heartbeat_time > 6)
				self->UpdateStatus(false);
			else
				self->UpdateStatus(true);
			return 0;

			return 10000;
		}, nullptr);
	}

	_last_heartbeat_time = t;
}


void SMSServer::UpdateStatus(bool flag)
{
	_connected = true;
}


bool SMSServer::IsConnected()
{
	return _connected;
}

std::string SMSServer::getWebrtcPublishApi()
{
	return "/api/v1/rtc/whip";
}

std::string SMSServer::getWebrtcPlayApi()
{
	return "/api/v1/rtc/play";
}

void SMSServer::OpenRtpServer(const std::string& str_ssrc, const std::string& streamId, int socket_type, int active, const std::function<void(int port)>& cb)
{
	std::string url = _base_url + "/api/v1/rtp/recv/create?active="+std::to_string(active)
		+"&ssrc=" + str_ssrc +"&socketType=" + std::to_string(socket_type) + "&appName=live&streamName=" + streamId;

	// shared_ptr<HttpClientApi> client = make_shared<HttpClientApi>(EventLoop::getCurrentLoop());
	//client->addHeader("Content-Type", "application/json;charset=UTF-8");
	// client->setMethod("GET");
	
	logInfo << "connect to utl: " << url;
	HttpClientApi::reportByHttp(url, "GET", "", [cb](const string& err, const json& res) {
		
		std::string str_res = res.dump();
		logInfo << "OpenRtpServer: " << res.dump();
		if (res.is_null() || str_res.empty() || !res.is_object() || res.find("port") == res.end())
			return;

		int port = res["port"].get<int>();
		//port = res.value("port", -1);

		if (cb) {
			cb(port);
		}
	});
}

void SMSServer::OpenRtpSender(const std::string& str_ssrc, const std::string& ip, int port, int socket_type, int active, const std::string& stream_id,
	const std::function<void(int port)>& cb)
{
	std::string url = _base_url + "/api/v1/rtp/send/create?active=" + std::to_string(active)
		+ "&ssrc=" + str_ssrc + "&port=" + std::to_string(port)
		+ "&socketType=" + std::to_string(socket_type) + "&ip=" + ip
		+ "&appName=live&streamName=" + stream_id;
	logInfo << "OpenRtpSender connect to utl: " << url;

	HttpClientApi::reportByHttp(url, "GET", "", [cb](const string& err, const json& res) {

		std::string str_res = res.dump();
		logInfo << "OpenRtpServer: " << res.dump();
		if (res.is_null() || str_res.empty())
			return;

		int port = res["port"].get<int>();
		//port = res.value("port", -1);

		if (cb) {
			cb(port);
		}

	});
}

void SMSServer::StartRtpSender(const std::string& str_ssrc, const std::string& ip, int port,
									const std::function<void()>& cb)
{
	std::string url = _base_url + "/api/v1/rtp/send/start?ssrc=" + str_ssrc + "&port=" + std::to_string(port)
		+ "&ip=" + ip;
	logInfo << "OpenRtpSender connect to utl: " << url;

	HttpClientApi::reportByHttp(url, "GET", "", [cb](const string& err, const json& res) {

		std::string str_res = res.dump();
		logInfo << "OpenRtpServer: " << res.dump();
		if (res.is_null() || str_res.empty())
			return;

		if (cb) {
			cb();
		}

	});
}

void SMSServer::CloseRtpSender(const std::string& stream_id)
{

}

void SMSServer::CloseRtpServer(const std::string& stream_id)
{
	// cpr::Response res = cpr::Get(
	// 	cpr::Url{ _base_url,"/index/api/closeRtpServer" },
	// 	cpr::Parameters{
	// 		{"secret",_info->Secret},
	// 		{"stream_id",stream_id}
	// 	},
	// 	cpr::Timeout{ 3s }
	// );

	// if (res.status_code == 200)
	// {

	// }
}


std::string SMSServer::ListRtpServer()
{
	// cpr::Response res = cpr::Get(
	// 	cpr::Url{ _base_url,"/index/api/listRtpServer" },
	// 	cpr::Parameters{ {"secret",_info->Secret} },
	// 	cpr::Timeout{ 3s }
	// );

	// if (res.status_code == 200)
	// {
	// 	return res.text;
	// }

	return "";
}

bool SMSServer::SinglePortMode()
{
	return _info->SinglePortMode;
}

int SMSServer::FixedRtpPort()
{
	return _info->SinglePortMode ? _info->RtpPort : -1;
}

int SMSServer::MaxPlayWaitTime()
{
	return _info->PlayWait;
}
