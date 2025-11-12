#ifndef GB28181Context_H
#define GB28181Context_H

#include "Buffer.h"
#include "Rtp/RtpPacket.h"
#include "Rtp/RtpSort.h"
#include "GB28181MediaSource.h"
#include "Util/TimeClock.h"

#include <memory>
#include <unordered_map>

// using namespace std;

class GB28181Context : public std::enable_shared_from_this<GB28181Context>
{
public:
    using Ptr = std::shared_ptr<GB28181Context>;
    using Wptr = std::weak_ptr<GB28181Context>;

    GB28181Context(const EventLoop::Ptr& loop, const std::string& uri, const std::string& vhost, const std::string& protocol, const std::string& type);
    ~GB28181Context();
public:
    bool init();
    void onRtpPacket(const RtpPacket::Ptr& rtp, struct sockaddr* addr = nullptr, int len = 0, bool sort = false);
    void heartbeat();
    bool isAlive() {return _alive;}

    void setPayloadType(const std::string& payloadType);
    void createVideoTrack(const std::string& videoCodec);
    void createAudioTrack(const std::string& audioCodec, int channel, int sampleBit, int sampleRate);

    void initAfterPublish();

private:
    bool _alive = true;
    int64_t _ssrc = -1;
    std::string _uri;
    std::string _vhost;
    std::string _protocol;
    std::string _type;
    std::string _payloadType = "ps";
    TimeClock _timeClock;
    std::shared_ptr<sockaddr> _addr;
    EventLoop::Ptr _loop;
    RtpSort::Ptr _sort;
    GB28181DecodeTrack::Ptr _videoTrack;
    GB28181DecodeTrack::Ptr _audioTrack;
    GB28181MediaSource::Wptr _source;
};



#endif //GB28181Context_H
