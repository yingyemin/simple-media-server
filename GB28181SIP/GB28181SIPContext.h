#ifndef GB28181SIPContext_H
#define GB28181SIPContext_H

#include "Buffer.h"
#include "Util/TimeClock.h"
#include "EventPoller/EventLoop.h"
#include "Net/Socket.h"
#include "SipMessage.h"

#include <memory>
#include <unordered_map>

using namespace std;

class MediaInfo{
public:
    string channelId;
    int channelNum;
    string ip;
    int port;
    uint32_t ssrc;
};

class GB28181SIPContext : public enable_shared_from_this<GB28181SIPContext>
{
public:
    using Ptr = shared_ptr<GB28181SIPContext>;
    using Wptr = weak_ptr<GB28181SIPContext>;

    GB28181SIPContext(const EventLoop::Ptr& loop, const string& deviceId, const string& vhost, const string& protocol, const string& type);
    ~GB28181SIPContext();
public:
    bool init();
    void onSipPacket(const Socket::Ptr& socket, const SipRequest::Ptr& req, struct sockaddr* addr = nullptr, int len = 0, bool sort = false);
    void heartbeat();
    bool isAlive() {return _alive;}
    void catalog();
    void invite(const MediaInfo& mediainfo);
    void bye(const string& channelId, const string& callId);
    void bye(const MediaInfo& mediainfo);
    void sendMessage(const char* msg, size_t size);

private:
    bool _alive = true;
    string _deviceId;
    string _vhost;
    string _protocol;
    string _type;
    string _payloadType = "ps";
    TimeClock _timeClock;
    SipStack _sipStack;
    shared_ptr<sockaddr> _addr;
    shared_ptr<SipRequest> _req = NULL;
    Socket::Ptr _socket;
    EventLoop::Ptr _loop;

    mutex _mtx;
    // channelId, callId, mediainfo
    unordered_map<string, unordered_map<string, MediaInfo>> _mapMediaInfo;
};



#endif //GB28181SIPContext_H
