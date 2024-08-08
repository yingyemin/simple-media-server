#ifndef GB28181Context_H
#define GB28181Context_H

#include "Buffer.h"
#include "Rtp/RtpPacket.h"
#include "Rtp/RtpSort.h"
#include "GB28181MediaSource.h"
#include "Util/TimeClock.h"

#include <memory>
#include <unordered_map>

using namespace std;

class GB28181Context : public enable_shared_from_this<GB28181Context>
{
public:
    using Ptr = shared_ptr<GB28181Context>;
    using Wptr = weak_ptr<GB28181Context>;

    GB28181Context(const EventLoop::Ptr& loop, const string& uri, const string& vhost, const string& protocol, const string& type);
    ~GB28181Context();
public:
    bool init();
    void onRtpPacket(const RtpPacket::Ptr& rtp, struct sockaddr* addr = nullptr, int len = 0, bool sort = false);
    void heartbeat();
    bool isAlive() {return _alive;}

private:
    bool _alive = true;
    string _uri;
    string _vhost;
    string _protocol;
    string _type;
    TimeClock _timeClock;
    shared_ptr<sockaddr> _addr;
    EventLoop::Ptr _loop;
    RtpSort::Ptr _sort;
    GB28181DecodeTrack::Ptr _track;
    GB28181MediaSource::Wptr _source;
};



#endif //GB28181Context_H
