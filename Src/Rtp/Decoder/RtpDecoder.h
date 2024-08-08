#ifndef RtpDecoder_H
#define RtpDecoder_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Common/Frame.h"
#include "Common/Track.h"
#include "Rtp/RtpPacket.h"

using namespace std;

class RtpDecoder : public enable_shared_from_this<RtpDecoder>
{
public:
    using Ptr = shared_ptr<RtpDecoder>;

    static RtpDecoder::Ptr creatDecoder(const shared_ptr<TrackInfo>& trackInfo);

    // virtual bool isStartGop(const RtpPacket::Ptr& rtp) = 0;
    virtual void decode(const RtpPacket::Ptr& rtp) = 0;
    virtual void setOnDecode(const function<void(const FrameBuffer::Ptr& frame)> cb) = 0;
};


#endif //RtpDecoder_H
