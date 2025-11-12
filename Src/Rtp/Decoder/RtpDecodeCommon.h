#ifndef RtpDecodeCommon_H
#define RtpDecodeCommon_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "RtpDecoder.h"
#include "Rtp/RtpPacket.h"
#include "Common/Track.h"

// using namespace std;

class RtpDecodeCommon : public RtpDecoder
{
public:
    using Ptr = std::shared_ptr<RtpDecodeCommon>;
    RtpDecodeCommon(const std::shared_ptr<TrackInfo>& trackInfo);

public:
    static bool isStartGop(const RtpPacket::Ptr& rtp) { return true; };
    void decode(const RtpPacket::Ptr& rtp) override;
    void setOnDecode(const std::function<void(const FrameBuffer::Ptr& frame)> cb) override;
    void onFrame(const FrameBuffer::Ptr& frame);

private:
    FrameBuffer::Ptr createFrame();

public:
    bool _firstRtp = true;
    bool _stage = 0; // 0 : new frame; 1 : part frame; 2 : loss rtp
    uint32_t _lastStamp = 0;
    uint16_t _lastSeq = -1;
    std::function<void(const FrameBuffer::Ptr& frame)> _onFrame;
    FrameBuffer::Ptr _frame;
    std::shared_ptr<TrackInfo> _trackInfo;
};


#endif //RtpDecodeCommon_H
