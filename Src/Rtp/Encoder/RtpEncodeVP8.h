#ifndef RtpEncodeVP8_H
#define RtpEncodeVP8_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "RtpEncoder.h"
#include "Common/Frame.h"
#include "Common/Track.h"
#include "Rtp/RtpPacket.h"

using namespace std;

class RtpEncodeVP8 : public RtpEncoder
{
public:
    using Ptr = shared_ptr<RtpEncodeVP8>;
    RtpEncodeVP8(const shared_ptr<TrackInfo>& trackInfo);

public:
    void encode(const FrameBuffer::Ptr& frame);
    void makeRtp(const char *data, size_t len, bool start, bool mark, bool gopStart, uint64_t pts);

    void setOnRtpPacket(const function<void(const RtpPacket::Ptr& packet, bool start)>& cb);
    void onRtpPacket(const RtpPacket::Ptr& packet, bool start);

private:
    uint64_t _lastPts = 0;
    uint16_t _lastSeq = 0;
    shared_ptr<TrackInfo> _trackInfo;
    function<void(const RtpPacket::Ptr& packet, bool start)> _onRtpPacket;
};


#endif //RtpEncodeVP8_H
