#ifndef RtpContext_H
#define RtpContext_H

#include "Buffer.h"
#include "Rtp/RtpPacket.h"
#include "Rtp/RtpSort.h"
#include "RtpMediaSource.h"
#include "Util/TimeClock.h"

#include <memory>
#include <unordered_map>

using namespace std;

class RtpContext : public enable_shared_from_this<RtpContext>
{
public:
    using Ptr = shared_ptr<RtpContext>;
    using Wptr = weak_ptr<RtpContext>;

    RtpContext(const EventLoop::Ptr& loop, const string& uri, const string& vhost, const string& protocol, const string& type);
    ~RtpContext();
public:
    bool init();
    void onRtpPacket(const RtpPacket::Ptr& rtp, struct sockaddr* addr = nullptr, int len = 0, bool sort = false);
    void heartbeat();
    bool isAlive() {return _alive;}

    void setPayloadType(const string& payloadType);
    void createVideoTrack(const string& videoCodec);
    void createAudioTrack(const string& audioCodec, int channel, int sampleBit, int sampleRate);

private:
    bool _alive = true;
    int64_t _ssrc = -1;
    string _uri;
    string _vhost;
    string _protocol;
    string _type;
    string _payloadType = "ps";
    TimeClock _timeClock;
    shared_ptr<sockaddr> _addr;
    EventLoop::Ptr _loop;
    RtpSort::Ptr _sort;
    RtpDecodeTrack::Ptr _videoTrack;
    RtpDecodeTrack::Ptr _audioTrack;
    RtpMediaSource::Wptr _source;
};



#endif //RtpContext_H
