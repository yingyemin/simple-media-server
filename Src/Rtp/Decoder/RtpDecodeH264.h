#ifndef RtpDecodeH264_H
#define RtpDecodeH264_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "RtpDecoder.h"
#include "Rtp/RtpPacket.h"
#include "Codec/H264Frame.h"

using namespace std;

class RtpDecodeH264 : public RtpDecoder
{
public:
    using Ptr = shared_ptr<RtpDecodeH264>;
    RtpDecodeH264(const shared_ptr<TrackInfo>& trackInfo);

public:
    static bool isStartGop(const RtpPacket::Ptr& rtp);
    void decode(const RtpPacket::Ptr& rtp) override;
    void setOnDecode(const function<void(const FrameBuffer::Ptr& frame)> cb) override;
    void onFrame(const FrameBuffer::Ptr& frame);

private:
    H264Frame::Ptr createFrame();
    void decodeStapA(const RtpPacket::Ptr& rtp);
    void decodeFuA(const RtpPacket::Ptr& rtp);
    void decodeSingle(const uint8_t *ptr, ssize_t size, uint64_t stamp);

public:
    bool _stage = 0; // 0 : new frame; 1 : part frame; 2 : loss rtp
    bool _firstRtp = true;
    uint32_t _lastStamp = -1;
    uint16_t _lastSeq = -1;
    function<void(const FrameBuffer::Ptr& frame)> _onFrame;
    H264Frame::Ptr _frame;
    shared_ptr<TrackInfo> _trackInfo;
};


#endif //RtpDecodeH264_H
