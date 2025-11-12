#ifndef RtpEncodeAV1_H
#define RtpEncodeAV1_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "RtpEncoder.h"
#include "Common/Frame.h"
#include "Common/Track.h"
#include "Rtp/RtpPacket.h"

// using namespace std;

class RtpEncodeAV1 : public RtpEncoder
{
public:
    using Ptr = std::shared_ptr<RtpEncodeAV1>;
    RtpEncodeAV1(const std::shared_ptr<TrackInfo>& trackInfo);

public:
    void encode(const FrameBuffer::Ptr& frame);
    void makeRtp(const char *data, size_t len, bool start, bool mark, bool gopStart, uint64_t pts, uint8_t aggregation);

    void setOnRtpPacket(const std::function<void(const RtpPacket::Ptr& packet, bool start)>& cb);
    void onRtpPacket(const RtpPacket::Ptr& packet, bool start);
    void encodeObu(const uint8_t* payload, size_t len, uint8_t& aggregation, 
                                bool gopStart, uint64_t pts, bool lastobu);

private:
    uint64_t _lastPts = 0;
    uint16_t _lastSeq = 0;
    std::shared_ptr<TrackInfo> _trackInfo;
    std::function<void(const RtpPacket::Ptr& packet, bool start)> _onRtpPacket;
};


#endif //RtpEncodeAV1_H
