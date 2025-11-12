#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
// #include <sys/socket.h>
// #include <arpa/inet.h>

#include "GB28181SIPContext.h"
#include "GB28181DeviceManager.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "Util/MD5.h"
#include "Common/Define.h"
#include "Common/Config.h"
#include "Common/HookManager.h"
#include "Utils.h"
#include "XmlParser.h"
#include "GB28181Hook.h"

using namespace std;

GB28181SIPContext::GB28181SIPContext(const EventLoop::Ptr& loop, const string& deviceId, const string& vhost, const string& protocol, const string& type)
    :_deviceId(deviceId)
    ,_vhost(vhost)
    ,_protocol(protocol)
    ,_type(type)
    ,_loop(loop)
{
    logTrace << "GB28181SIPContext::GB28181SIPContext";
    _req.reset(new SipRequest());
    _req->serial = Config::instance()->get("SipServer", "Server", "id");
    _req->realm = Config::instance()->get("SipServer", "Server", "realm");
    _req->host = Config::instance()->get("SipServer", "Server", "ip");
    _req->host_port = Config::instance()->get("SipServer", "Server", "port");
    _req->sip_auth_id = Config::instance()->get("SipServer", "Server", "id");
    _req->sip_auth_pwd = Config::instance()->get("SipServer", "Server", "password");
}

GB28181SIPContext::~GB28181SIPContext()
{
    logTrace << "GB28181SIPContext::~GB28181SIPContext";
    GB28181DeviceManager::instance()->removeDevice(_deviceId);
}

bool GB28181SIPContext::init()
{
    _deviceInfo = make_shared<DeviceInfo>();
    _deviceInfo->_regist_time = TimeClock::now();
    _deviceInfo->_last_time = _deviceInfo->_regist_time;
    _deviceInfo->_registered = true;
    _deviceInfo->_device_id = _deviceId;

    GB28181DeviceManager::instance()->addDevice(shared_from_this());

    weak_ptr<GB28181SIPContext> wSelf = shared_from_this();
    _loop->addTimerTask(2000, [wSelf](){
        auto self = wSelf.lock();
        if (self) {
            self->heartbeat();
            return 2000;
        }

        return 0;
    }, nullptr);

    string ip = Config::instance()->get("LocalIp");
    int port = Config::instance()->get("Http", "Api", "Api1", "port");

    _serverId = ip + ":" + to_string(port);

    return true;
}

void GB28181SIPContext::onSipPacket(const Socket::Ptr& socket, const SipRequest::Ptr& req, struct sockaddr* addr, int len, bool sort)
{
    _deviceInfo->_last_time = TimeClock::now();

    if (addr) {
        if (!_addr || _alive == false) {
            _addr = make_shared<sockaddr>();
            memcpy(_addr.get(), addr, sizeof(sockaddr));
            if (addr) {
                char buf[INET_ADDRSTRLEN] = "";
                _req->peer_port = ntohs(((sockaddr_in*)&addr)->sin_port);
                inet_ntop(AF_INET, &(((sockaddr_in*)&addr)->sin_addr), buf, INET_ADDRSTRLEN);
                _req->peer_ip.assign(buf);
            } else {
                _req->peer_ip = socket->getPeerIp();
                _req->peer_port = socket->getPeerPort();
            }
        } else if (memcmp(_addr.get(), addr, sizeof(struct sockaddr)) != 0) {
            // 记录一下这个流，提供切换流的api
            logWarn<< "收到 sip 包，但已经存在一个相同的设备，忽略";
            return ;
        }
    }

    if (_socket != socket) {
        _socket = socket;
    }

    if (!_alive && !req->is_register()) {
        logInfo << "GB28181SIPContext::onSipPacket isAlive is false";
        return ;
    }

    stringstream ss;
    if (req->is_register()) {
        handleRegister(req, ss);
    } else if (req->is_message()) {
        handleMessage(req, ss);
    } else if (req->is_invite()) {        
        handleInvite(req, ss);
    } else if (req->is_bye()) {
        _sipStack.resp_status(ss, req);
    } else if (req->is_notify()) {
        handleNotify(req, ss);
        _sipStack.resp_status(ss, req);
    }

    sendMessage(ss.str().data(), ss.str().size());

    _timeClock.update();
}

void GB28181SIPContext::heartbeat()
{
    // static int timeout = Config::instance()->getConfig()["GB28181"]["Server"]["timeout"];
    static int timeout = Config::instance()->getAndListen([](const json& config){
        timeout = Config::instance()->get("SipServer", "Server", "timeout", "", "15000");
        logInfo << "timeout: " << timeout;
    }, "SipServer", "Server", "timeout", "", "15000");

    static int catalogTimeout = Config::instance()->getAndListen([](const json& config){
        catalogTimeout = Config::instance()->get("SipServer", "Server", "catalogTimeout", "", "15000");
        logInfo << "catalogTimeout: " << catalogTimeout;
    }, "SipServer", "Server", "catalogTimeout", "", "15000");

    static int keepAliveTimeout = Config::instance()->getAndListen([](const json& config){
        keepAliveTimeout = Config::instance()->get("SipServer", "Server", "keepAliveTimeout", "", "30000");
        logInfo << "keepAliveTimeout: " << keepAliveTimeout;
    }, "SipServer", "Server", "keepAliveTimeout", "", "30000");

    if (_timeClock.startToNow() > timeout) {
        logInfo << "alive is false";
        _alive = false;
    }

    if (_catalogTime.startToNow() > catalogTimeout) {
        _catalogFinish = true;
    }

    if (_keepAliveTime.startToNow() > keepAliveTimeout) {
        logInfo << "keepAliveTimeout is false";
        _alive = false;
    }
}

void GB28181SIPContext::catalog()
{
    if (!isAlive()) {
        logInfo << "GB28181SIPContext::catalog isAlive is false";
        return;
    }

    if (!_catalogFinish) {
        logInfo << "GB28181SIPContext::catalog _catalogFinish is false";
        return ;
    }

    auto req = make_shared<SipRequest>();
    req->host = _req->host;
    req->host_port = _req->host_port;
    req->realm = _req->realm;
    req->serial = _deviceId;
    req->sip_auth_id = _deviceId;

    std::stringstream ss;
    _sipStack.req_query_catalog(ss, req, ++_catalogSn);

    sendMessage(ss.str().data(), ss.str().size());
}

void GB28181SIPContext::deviceInfo()
{
    if (!isAlive()) {
        logInfo << "GB28181SIPContext::catalog isAlive is false";
        return;
    }

    auto req = make_shared<SipRequest>();
    req->host = _req->host;
    req->host_port = _req->host_port;
    req->realm = _req->realm;
    req->serial = _deviceId;
    req->sip_auth_id = _deviceId;

    std::stringstream ss;
    _sipStack.req_query_deviceInfo(ss, req);

    // TODO 记住SN，根据SN收包

    sendMessage(ss.str().data(), ss.str().size());
}

void GB28181SIPContext::invite(const MediaInfo& mediainfo)
{
    if (!isAlive()) {
        logInfo << "GB28181SIPContext::catalog isAlive is false";
        return;
    }

    auto req = make_shared<SipRequest>();
    req->host = _req->host;
    req->host_port = _req->host_port;
    req->realm = _req->realm;
    req->serial = _req->serial;
    req->sip_auth_id = mediainfo.channelId;

    std::stringstream ss;
    string callId = _sipStack.req_invite(ss, req, mediainfo.ip, mediainfo.port, mediainfo.ssrc);

    sendMessage(ss.str().data(), ss.str().size());

    logInfo << "channelId:" << mediainfo.channelId << ", callid:" << callId << this;
    lock_guard<mutex> lck(_mtx);
    _mapMediaInfo[mediainfo.channelId][callId] = mediainfo;
}

void GB28181SIPContext::bye(const string& channelId, const string& callId)
{
    auto req = make_shared<SipRequest>();
    req->host = _req->host;
    req->host_port = _req->host_port;
    req->realm = _req->realm;
    req->serial = _req->serial;
    req->sip_auth_id = channelId;
    req->call_id = callId;

    std::stringstream ss;
    _sipStack.req_bye(ss, req);

    sendMessage(ss.str().data(), ss.str().size());

    logInfo << "channelId:" << channelId << ", callid:" << callId;
    lock_guard<mutex> lck(_mtx);
    _mapMediaInfo[channelId].erase(callId);
}

void GB28181SIPContext::bye(const MediaInfo& mediainfo)
{
    bool findFlag = false;
    MediaInfo targetInfo;
    string callId;
    {
        lock_guard<mutex> lck(_mtx);
        if (_mapMediaInfo.find(mediainfo.channelId) == _mapMediaInfo.end()) {
            return ;
        }

        for (auto& info : _mapMediaInfo[mediainfo.channelId]) {
            if (info.second.channelNum == mediainfo.channelNum
                && info.second.ip == mediainfo.ip
                && info.second.port == mediainfo.port
                && info.second.ssrc == mediainfo.ssrc)
            {
                findFlag = true;
                targetInfo = info.second;
                callId = info.first;
                break;
            }
        }
    }

    if (!findFlag) {
        return ;
    }

    auto req = make_shared<SipRequest>();
    req->host = _req->host;
    req->host_port = _req->host_port;
    req->realm = _req->realm;
    req->serial = _req->serial;
    req->sip_auth_id = targetInfo.channelId;
    req->call_id = callId;

    std::stringstream ss;
    _sipStack.req_bye(ss, req);

    sendMessage(ss.str().data(), ss.str().size());

    logInfo << "channelId:" << mediainfo.channelId << ", callid:" << callId;
    lock_guard<mutex> lck(_mtx);
    _mapMediaInfo[mediainfo.channelId].erase(callId);
}

void GB28181SIPContext::sendMessage(const char* msg, size_t size)
{
    logTrace << "send a message" << msg;
    _socket->send(msg, size, 1, _addr.get(), sizeof(sockaddr));
}

void GB28181SIPContext::handleNotify(const SipRequest::Ptr& req, std::stringstream& ss)
{
    logInfo << "sip message: " << req->content;
    logInfo << "sip method: " << req->method;
    XmlParser parser;
    pugi::xml_document doc;
    auto ret = parser.Parse(req->content.data(), (int)req->content.length(), doc);
    if (!ret)
    {
        _sipStack.resp_bad_request(ss, req);
        return ;
    }

    manscdp_msgbody_header_t header;
    ret = parser.ParseHeader(header, doc);
    if (!ret)
    {
        _sipStack.resp_bad_request(ss, req);
        return ;
    }
    
    if (req->cmdtype == SipCmdRequest) {
        if (header.cmd_type == MANSCDP_QUERY_CMD_CATALOG) {
            handleNotifyCatalog(doc);
        } else {
            logWarn << "unknown sip message method:" << req->method;
        }
        _sipStack.resp_status(ss, req);
    }
}

void GB28181SIPContext::handleRegister(const SipRequest::Ptr& req, std::stringstream& ss)
{
    if (req->authorization.empty()) {
        logInfo << "收到未授权的注册请求";
        _sipStack.resp_status(ss, req);
    } else {
        logInfo << "收到授权的注册请求";
        static string password = Config::instance()->getAndListen([](const json &config){
            password = Config::instance()->get("SipServer", "Server","password");
        }, "SipServer", "Server","password");

        if (!startWith(req->authorization, "Digest ")) {
            _sipStack.resp_bad_request(ss, req);
            return ;
        }

        auto authStr = req->authorization.substr(strlen("Digest "));
        auto vecParams = split(authStr, ",");
        string username, realm, nonce, uri, method, response, qop, cnonce, nc;
        for (auto &param : vecParams) {
            auto vec = split(param, "=");
            if (vec.size() != 2) {
                continue;
            }
            auto key = trim(vec[0], " ");
            if (key == "username") {
                username = trim(vec[1], "\"");
            } else if (key == "realm") {
                realm = trim(vec[1], "\"");
            } else if (key == "nonce") {
                nonce = trim(vec[1], "\"");
            } else if (key == "uri") {
                uri = trim(vec[1], "\"");
            } else if (key == "method") {
                method = trim(vec[1], "\"");
            } else if (key == "response") {
                response = trim(vec[1], "\"");
            } else if (key == "qop") {
                qop = trim(vec[1], "\"");
            } else if (key == "cnonce") {
                cnonce = trim(vec[1], "\"");
            } else if (key == "nc") {
                nc = trim(vec[1], "\"");
            }
        }
        method = "REGISTER";
        string encPwd = MD5(std::string(username) + ":" + realm + ":" + password).hexdigest();

        auto encRes = MD5(encPwd + ":" + nonce + ":" + nc + ":" + cnonce + ":" + qop + ":" + MD5(string(req->method) + ":" + uri).hexdigest()).hexdigest();

        logInfo << "MD5 encPwd: " << encPwd;
        logInfo << "MD5 nonce: " << nonce;
        logInfo << "MD5 method: " << method;
        logInfo << "MD5 uri: " << uri;
        logInfo << "MD5 username: " << username;
        logInfo << "MD5 password: " << password;
        logInfo << "MD5 realm: " << realm;
        logInfo << "MD5 encRes: " << encRes;
        logInfo << "response: " << response;

        if (encRes != response) {
            _sipStack.resp_401_unauthorized(ss, req);
            return ;
        }

        _sipStack.resp_status(ss, req);

        _timeClock.update();

        string status;
        if (req->expires == 0) {
            _alive = false;
            _deviceInfo->_registered = false;
            status = "off";
        } else {
            _alive = true;
            status = "on";
            _deviceInfo->_registered = true;
            catalog();
            deviceInfo();
        }
        
        DeviceStatusInfo info;
        info.deviceId = _deviceId;
        info.serverId = _serverId;
        info.status = status;
        GB28181Hook::instance()->onDeviceStatus(info);
        return ;
    }
}

void GB28181SIPContext::handleMessage(const SipRequest::Ptr& req, std::stringstream& ss)
{
    logInfo << "sip message: " << req->content;
    logInfo << "sip method: " << req->method;
    XmlParser parser;
    pugi::xml_document doc;
    auto ret = parser.Parse(req->content.data(), (int)req->content.length(), doc);
    if (!ret)
    {
        _sipStack.resp_bad_request(ss, req);
        return ;
    }

    manscdp_msgbody_header_t header;
    ret = parser.ParseHeader(header, doc);
    if (!ret)
    {
        _sipStack.resp_bad_request(ss, req);
        return ;
    }
    
    if (req->cmdtype == SipCmdRequest) {
        if (header.cmd_type == MANSCDP_QUERY_CMD_CATALOG) {
            handleCatalog(doc);
        } else if (header.cmd_type == MANSCDP_QUERY_CMD_DEVICE_INFO) {
            handleDeviceInfo(doc);
        } else if (header.cmd_type == MANSCDP_QUERY_CMD_DEVICE_STATUS) {

        } else if (header.cmd_type == MANSCDP_QUERY_CMD_RECORD_INFO) {

        } else if (header.cmd_type == MANSCDP_QUERY_CMD_CONFIG_DOWNLOAD) {

        } else if (header.cmd_type == MANSCDP_QUERY_CMD_PRESET_QUERY) {

        } else if (header.cmd_type == MANSCDP_NOTIFY_CMD_KEEPALIVE) {
            DeviceHeartbeatInfo info;
            info.deviceId = _deviceId;
            info.serverId = _serverId;
            info.timestamp = TimeClock::now();
            GB28181Hook::instance()->onDeviceHeartbeat(info);

            _keepAliveTime.update();
            _deviceInfo->_last_time = TimeClock::now();
            _deviceInfo->_registered = true;
        } else {
            logWarn << "unknown sip message method:" << req->method;
        }
        _sipStack.resp_status(ss, req);
    }
}

void GB28181SIPContext::handleInvite(const SipRequest::Ptr& req, std::stringstream& ss)
{
    if (req->cmdtype == SipCmdRespone){
        if (req->status == "200") {
            req->realm = _req->realm;
            req->serial = _req->serial;
            req->host = _req->host;
            req->host_port = _req->host_port;
            _sipStack.resp_ack(ss, req);
        }else if (req->status == "100") {
            
        }else{
            _sipStack.resp_bad_request(ss, req);
        }
    }
}

void GB28181SIPContext::handleCatalog(const pugi::xml_document& doc)
{
    auto root = doc.first_child();

	auto device_id = root.child("DeviceID").text().as_string();
	if (device_id != _deviceId)
	{
		return ;
	}

	auto node = root.child("DeviceList");
	if (node.empty())
	{
		return ;
	}

    int sn = root.child("SN").text().as_ullong();
    if (sn != _catalogSn) {
        return ;
    }

    int deviceNum = root.child("DeviceList").attribute("Num").as_int();
    int childrenNum = 0;

    _deviceInfo->_channel_count = root.child("SumNum").text().as_uint();

	auto children = node.children("Item");
	for (auto&& child : children)
	{
		auto channel_id = child.child("DeviceID").text().as_string();

		bool find = false;
		std::shared_ptr<Channel> channel = nullptr;
        {
            lock_guard<mutex> lck(_mtxChannel);
            if (_mapChannel.find(channel_id) != _mapChannel.end()) {
                channel = _mapChannel[channel_id];
            } else {
                channel = make_shared<Channel>();
                _mapChannel[channel_id] = channel;
            }
        }

		channel->SetChannelID(child.child("DeviceID").text().as_string());
		channel->SetName(child.child("Name").text().as_string());
		channel->SetManufacturer(child.child("Manufacturer").text().as_string());
		channel->SetModel(child.child("Model").text().as_string());
		channel->SetOwner(child.child("Owner").text().as_string());
		channel->SetCivilCode(child.child("CivilCode").text().as_string());
		channel->SetAddress(child.child("Address").text().as_string());
		channel->SetParental(child.child("Parental").text().as_string());
		channel->SetParentID(child.child("ParentID").text().as_string());
		channel->SetRegisterWay(child.child("RegisterWay").text().as_string());
		channel->SetSecrety(child.child("Secrecy").text().as_string());
		channel->SetStreamNum(child.child("StreamNum").text().as_string());
		channel->SetIpAddress(child.child("IPAddress").text().as_string());
		channel->SetStatus(child.child("Status").text().as_string());
		
		// 增加ssrc
		channel->SetDefaultSSRC("10000");
		auto info = child.child("Info");
		if (info && info.child("PTZType"))
		{
			channel->SetPtzType(info.child("PTZType").text().as_string());
		}
		if (info && info.child("DownloadSpeed"))
		{
			channel->SetDownloadSpeed(info.child("DownloadSpeed").text().as_string());
		}

        ++childrenNum;
        
        DeviceChannelInfo channelInfo;
        channelInfo.channelId = child.child("DeviceID").text().as_string();
        channelInfo.name = child.child("Name").text().as_string();
        channelInfo.paraentId = child.child("ParentID").text().as_string();
        GB28181Hook::instance()->onDeviceChannel(_deviceId, _serverId, channelInfo);
	}

    if (deviceNum != childrenNum) {
        logWarn << "deviceNum:" << deviceNum << ", childrenNum:" << childrenNum;
    }
}

void GB28181SIPContext::handleNotifyCatalog(const pugi::xml_document& doc)
{
    auto root = doc.first_child();

	auto device_id = root.child("DeviceID").text().as_string();
	if (device_id != _deviceId)
	{
		return ;
	}

	auto node = root.child("DeviceList");
	if (node.empty())
	{
		return ;
	}

    int sn = root.child("SN").text().as_ullong();
    if (sn != _notifyCatalogSn) {
        return ;
    }

    int deviceNum = root.child("DeviceList").attribute("Num").as_int();
    int childrenNum = 0;

    _deviceInfo->_channel_count = root.child("SumNum").text().as_uint();

	auto children = node.children("Item");
	for (auto&& child : children)
	{
        ++childrenNum;

		auto channel_id = child.child("DeviceID").text().as_string();
        std::string event = child.child("Event").text().as_string();
        if (event.empty()) {
            continue;
        } else if (strcasecmp(event.c_str(), "off") == 0) {
            event = "off";
        }

		bool find = false;
		std::shared_ptr<Channel> channel = nullptr;
        {
            lock_guard<mutex> lck(_mtxChannel);
            if (_mapChannel.find(channel_id) != _mapChannel.end()) {
                channel = _mapChannel[channel_id];
            } else if (event != "off") {
                channel = make_shared<Channel>();
                _mapChannel[channel_id] = channel;
            }
        }

        DeviceChannelInfo channelInfo;
        if (event == "off") {
            if (channel) {
                channelInfo.channelId = channel_id;
                channelInfo.name = channel->GetName();
                channelInfo.paraentId = channel->GetParentID();
                GB28181Hook::instance()->onDeviceChannel(_deviceId, _serverId, channelInfo);

                lock_guard<mutex> lck(_mtxChannel);
                _mapChannel.erase(channel_id);
            }
        } else {
            channel->SetChannelID(child.child("DeviceID").text().as_string());
            channel->SetName(child.child("Name").text().as_string());
            channel->SetManufacturer(child.child("Manufacturer").text().as_string());
            channel->SetModel(child.child("Model").text().as_string());
            channel->SetOwner(child.child("Owner").text().as_string());
            channel->SetCivilCode(child.child("CivilCode").text().as_string());
            channel->SetAddress(child.child("Address").text().as_string());
            channel->SetParental(child.child("Parental").text().as_string());
            channel->SetParentID(child.child("ParentID").text().as_string());
            channel->SetRegisterWay(child.child("RegisterWay").text().as_string());
            channel->SetSecrety(child.child("Secrecy").text().as_string());
            channel->SetStreamNum(child.child("StreamNum").text().as_string());
            channel->SetIpAddress(child.child("IPAddress").text().as_string());
            channel->SetStatus(child.child("Status").text().as_string());

            // 增加ssrc
            channel->SetDefaultSSRC("10000");
            auto info = child.child("Info");
            if (info && info.child("PTZType"))
            {
                channel->SetPtzType(info.child("PTZType").text().as_string());
            }
            if (info && info.child("DownloadSpeed"))
            {
                channel->SetDownloadSpeed(info.child("DownloadSpeed").text().as_string());
            }

            channelInfo.channelId = child.child("DeviceID").text().as_string();
            channelInfo.name = child.child("Name").text().as_string();
            channelInfo.paraentId = child.child("ParentID").text().as_string();
            GB28181Hook::instance()->onDeviceChannel(_deviceId, _serverId, channelInfo);
        }
	}

    if (deviceNum != childrenNum) {
        logWarn << "deviceNum:" << deviceNum << ", childrenNum:" << childrenNum;
    }
}

void GB28181SIPContext::handleDeviceInfo(const pugi::xml_document& doc)
{
    auto root = doc.first_child();

	auto device_id = root.child("DeviceID").text().as_string();
    if (device_id != _deviceId) {
        logError << "get a invalid deviceId: " << device_id << ", origin deviceId: " << _deviceId;
        return ;
    }
    
    _deviceInfo->_name = ToMbcsString(root.child("DeviceName").text().as_string());
    _deviceInfo->_manufacturer = ToMbcsString(root.child("Manufacturer").text().as_string());
    _deviceInfo->_model = root.child("Model").text().as_string();

    DeviceInfoInfo  info;
    info.channelCount = _deviceInfo->_channel_count;
    info.deviceId = _deviceId;
    info.name = _deviceInfo->_name;
    info.manufacturer = _deviceInfo->_manufacturer;
    info.serverId = _serverId;
    GB28181Hook::instance()->onDeviceInfo(info);
}

unordered_map<string, Channel::Ptr> GB28181SIPContext::getChannelList()
{
    lock_guard<mutex> lck(_mtxChannel);
    return _mapChannel;
}

std::unordered_map<std::string, std::unordered_map<std::string, MediaInfo>> GB28181SIPContext::getMediaInfoList()
{
    lock_guard<mutex> lck(_mtx);
    return _mapMediaInfo;
}
