#include "SipServer.h"
#include "Http/HttpClientApi.h"
#include "Log/Logger.h"
#include "EventPoller/EventLoopPool.h"

#include "Common/Config.h"

using namespace std;

std::shared_ptr<SipServer> SipServer::instance()
{
	static std::shared_ptr<SipServer> manager(new SipServer());
	return manager;
}

void SipServer::Init()
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

void SipServer::addServer(const std::string& serverId, const SipServer::Ptr& server)
{
	logDebug << "addServer: " << serverId;
	{
		std::lock_guard<mutex> g(_mutex);
		_server_map[serverId] = server;
	}
}

void SipServer::removeServer(const std::string& serverId)
{
	logDebug << "removeServer: " << serverId;
	std::lock_guard<mutex> g(_mutex);
	_server_map.erase(serverId);
}

SipServer::Ptr SipServer::getServer(const std::string& serverId)
{
	logDebug << "getServer: " << serverId;
	std::lock_guard<mutex> g(_mutex);
	auto it = _server_map.find(serverId);
	if (it != _server_map.end())
		return it->second;
	return nullptr;
}

SipServer::Ptr SipServer::getServerByCircly()
{
	std::lock_guard<mutex> g(_mutex);
	int i = 0;
	SipServer::Ptr lastOnline;
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

std::unordered_map<std::string, SipServer::Ptr> SipServer::getServerList()
{
	std::lock_guard<mutex> g(_mutex);
	return _server_map;
}

/////////////////////////////////////////////////////////////////////////////

void SipServer::UpdateHeartbeatTime(time_t t)
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
		std::weak_ptr<SipServer> wSelf = shared_from_this();
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


void SipServer::UpdateStatus(bool flag)
{
	_connected = true;
}


bool SipServer::IsConnected()
{
	return _connected;
}

void SipServer::invite(const std::string& deviceId, const std::string& channelId, int channelNum, 
					const std::string& ip, int port, const std::string& ssrc)
{
	std::string url = _base_url + "/api/v1/device/invite?deviceId=" + deviceId
		+ "&ssrc=" + ssrc +"&channelId=" + channelId + "&channelNum=" + to_string(channelNum) 
		+ "&ip=" + ip + "&port=" + to_string(port);

	// shared_ptr<HttpClientApi> client = make_shared<HttpClientApi>(EventLoop::getCurrentLoop());
	//client->addHeader("Content-Type", "application/json;charset=UTF-8");
	// client->setMethod("GET");
	
	logInfo << "connect to utl: " << url;
	HttpClientApi::reportByHttp(url, "GET", "", [](const string& err, const json& res) {
		
		std::string str_res = res.dump();
		logInfo << "device invite: " << res.dump();
		if (res.is_null() || str_res.empty() || !res.is_object())
			return;

		if (res["code"] != "200") {
			// TODO close stream
		}
	});
}
