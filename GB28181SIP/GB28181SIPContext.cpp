#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "GB28181SIPContext.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"
#include "Common/Config.h"
#include "Hook/MediaHook.h"

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
}

bool GB28181SIPContext::init()
{
    
    return true;
}

void GB28181SIPContext::onSipPacket(const Socket::Ptr& socket, const SipRequest::Ptr& req, struct sockaddr* addr, int len, bool sort)
{
    if (addr) {
        if (!_addr) {
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

    stringstream ss;
    if (req->is_register()) {
        if (req->authorization.empty()) {
            _sipStack.resp_status(ss, req);
        } else {
            // TODO auth
            _sipStack.resp_status(ss, req);
        }
    } else if (req->is_message()) {
        if (req->cmdtype == SipCmdRequest) {
            _sipStack.resp_status(ss, req);
        }
    } else if (req->is_invite()) {
        MediaInfo info;
        {
            lock_guard<mutex> lck(_mtx);
            if (_mapMediaInfo[req->sip_auth_id].count(req->call_id) > 0) {
                info = _mapMediaInfo[req->sip_auth_id][req->call_id];
            } else {
                _sipStack.resp_ack(ss, req);
                sendMessage(ss.str().data(), ss.str().size());
                return ;
            }
        }
        
        if (req->cmdtype == SipCmdRespone){
            if (req->status == "200") {
                _sipStack.resp_ack(ss, req);
            }else if (req->status == "100") {
                
            }else{

            }
        }
       
    } else if (req->is_bye()) {
        _sipStack.resp_status(ss, req);
    }

    sendMessage(ss.str().data(), ss.str().size());

    _timeClock.update();
}

void GB28181SIPContext::heartbeat()
{
    // static int timeout = Config::instance()->getConfig()["GB28181"]["Server"]["timeout"];
    static int timeout = Config::instance()->getAndListen([](const json& config){
        timeout = config["GB28181"]["Server"]["timeout"];
        logInfo << "timeout: " << timeout;
    }, "GB28181", "Server", "timeout");

    if (_timeClock.startToNow() > timeout) {
        logInfo << "alive is false";
        _alive = false;
    }
}

void GB28181SIPContext::catalog()
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
    _sipStack.req_query_catalog(ss, req);

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

    lock_guard<mutex> lck(_mtx);
    _mapMediaInfo[mediainfo.channelId].erase(callId);
}

void GB28181SIPContext::sendMessage(const char* msg, size_t size)
{
    logTrace << "send a message" << msg;
    _socket->send(msg, size, 1, _addr.get(), sizeof(sockaddr));
}