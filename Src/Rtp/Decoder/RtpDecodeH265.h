#ifndef RtpDecodeH265_H
#define RtpDecodeH265_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "RtpDecoder.h"
#include "Rtp/RtpPacket.h"
#include "Codec/H265Frame.h"

// using namespace std;

class RtpDecodeH265 : public RtpDecoder
{
public:
    using Ptr = std::shared_ptr<RtpDecodeH265>;
    RtpDecodeH265(const std::shared_ptr<TrackInfo>& trackInfo);

public:
    static bool isStartGop(const RtpPacket::Ptr& rtp);
    void decode(const RtpPacket::Ptr& rtp) override;
    void setOnDecode(const std::function<void(const FrameBuffer::Ptr& frame)> cb) override;
    void onFrame(const FrameBuffer::Ptr& frame);

private:
    H265Frame::Ptr createFrame();
    void decodeStapA(const RtpPacket::Ptr& rtp);
    void decodeFuA(const RtpPacket::Ptr& rtp);
    void decodeSingle(const uint8_t *ptr, ssize_t size, uint64_t stamp);

public:
    bool _stage = 0; // 0 : new frame; 1 : part frame; 2 : loss rtp
    bool _firstRtp = true;
    uint32_t _lastStamp = -1;
    uint16_t _lastSeq = -1;
    std::function<void(const FrameBuffer::Ptr& frame)> _onFrame;
    H265Frame::Ptr _frame;
    std::shared_ptr<TrackInfo> _trackInfo;
};


#endif //RtpDecodeH265_H
