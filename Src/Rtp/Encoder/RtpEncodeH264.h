#ifndef RtpEncodeH264_H
#define RtpEncodeH264_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "RtpEncoder.h"
#include "Common/Frame.h"
#include "Common/Track.h"
#include "Rtp/RtpPacket.h"

using namespace std;

class RtpEncodeH264 : public RtpEncoder
{
public:
    using Ptr = shared_ptr<RtpEncodeH264>;
    RtpEncodeH264(const shared_ptr<TrackInfo>& trackInfo);

public:
    void encode(const FrameBuffer::Ptr& frame);
    void encodeFuA(const FrameBuffer::Ptr& frame);
    void encodeSingle(const FrameBuffer::Ptr& frame);

    void setOnRtpPacket(const function<void(const RtpPacket::Ptr& packet, bool start)>& cb);
    void onRtpPacket(const RtpPacket::Ptr& packet, bool start);

private:
    bool _first = true;
    uint64_t _lastPts = 0;
    uint16_t _lastSeq = 100;
    shared_ptr<TrackInfo> _trackInfo;
    function<void(const RtpPacket::Ptr& packet, bool start)> _onRtpPacket;
};


#endif //RtpEncodeH264_H
