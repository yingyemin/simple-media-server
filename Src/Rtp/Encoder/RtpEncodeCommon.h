#ifndef RtpEncodeCommon_H
#define RtpEncodeCommon_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "RtpEncoder.h"
#include "Common/Frame.h"
#include "Common/Track.h"
#include "Rtp/RtpPacket.h"

// using namespace std;

class RtpEncodeCommon : public RtpEncoder
{
public:
    using Ptr = std::shared_ptr<RtpEncodeCommon>;
    RtpEncodeCommon(const std::shared_ptr<TrackInfo>& trackInfo);

public:
    void encode(const FrameBuffer::Ptr& frame);
    void makeRtp(const char *data, size_t len, bool mark, uint64_t stamp);

    void setOnRtpPacket(const std::function<void(const RtpPacket::Ptr& packet, bool start)>& cb);
    void onRtpPacket(const RtpPacket::Ptr& packet, bool start);

private:
    uint64_t _lastPts = 0;
    uint16_t _lastSeq = 0;
    std::shared_ptr<TrackInfo> _trackInfo;
    std::function<void(const RtpPacket::Ptr& packet, bool start)> _onRtpPacket;
};


#endif //RtpEncodeCommon_H
