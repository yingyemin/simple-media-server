#ifndef RtpEncoder_H
#define RtpEncoder_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Common/Frame.h"
#include "Common/Track.h"
#include "Rtp/RtpPacket.h"

using namespace std;

class RtpEncoder : public enable_shared_from_this<RtpEncoder>
{
public:
    using Ptr = shared_ptr<RtpEncoder>;

    static RtpEncoder::Ptr create(const shared_ptr<TrackInfo>& trackInfo);

    virtual void setOnRtpPacket(const function<void(const RtpPacket::Ptr& packet, bool start)>& cb) = 0;
    virtual void encode(const FrameBuffer::Ptr& frame) = 0;
    virtual void setSsrc(uint32_t ssrc) {_ssrc = ssrc;}

protected:
    uint32_t _ssrc = 0;
};


#endif //RtpEncoder_H
