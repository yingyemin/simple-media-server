#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <sys/socket.h>

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

}

GB28181SIPContext::~GB28181SIPContext()
{
}

bool GB28181SIPContext::init()
{
    
    return true;
}

void GB28181SIPContext::onSipPacket(const SipRequest::Ptr& rtp, struct sockaddr* addr, int len, bool sort)
{
    if (addr) {
        if (!_addr) {
            _addr = make_shared<sockaddr>();
            memcpy(_addr.get(), addr, sizeof(sockaddr));
        } else if (memcmp(_addr.get(), addr, sizeof(struct sockaddr)) != 0) {
            // 记录一下这个流，提供切换流的api
            return ;
        }
    }

    

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