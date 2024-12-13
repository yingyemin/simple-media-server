#ifndef RtpEncodeMp3_H
#define RtpEncodeMp3_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "RtpEncoder.h"
#include "Common/Frame.h"
#include "Common/Track.h"
#include "Rtp/RtpPacket.h"

using namespace std;

class RtpEncodeMp3 : public RtpEncoder
{
public:
    using Ptr = shared_ptr<RtpEncodeMp3>;
    RtpEncodeMp3(const shared_ptr<TrackInfo>& trackInfo);

public:
    void encode(const FrameBuffer::Ptr& frame);
    void makeRtp(const char *data, size_t len, bool mark, uint64_t stamp, uint64_t offset);

    void setOnRtpPacket(const function<void(const RtpPacket::Ptr& packet, bool start)>& cb);
    void onRtpPacket(const RtpPacket::Ptr& packet, bool start);

private:
    uint64_t _lastPts = 0;
    uint16_t _lastSeq = 0;
    shared_ptr<TrackInfo> _trackInfo;
    function<void(const RtpPacket::Ptr& packet, bool start)> _onRtpPacket;
};


#endif //RtpEncodeMp3_H
