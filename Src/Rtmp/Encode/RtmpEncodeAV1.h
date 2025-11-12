#ifndef RtmpEncodeAV1_h
#define RtmpEncodeAV1_h

#include <memory>

#include "Rtmp/RtmpMessage.h"
#include "Common/Frame.h"
#include "RtmpEncode.h"

// using namespace std;

class RtmpEncodeAV1 : public RtmpEncode, public std::enable_shared_from_this<RtmpEncodeAV1> {
public:
    using Ptr = std::shared_ptr<RtmpEncodeAV1>;
    using Wptr = std::weak_ptr<RtmpEncodeAV1>;

    RtmpEncodeAV1(const std::shared_ptr<TrackInfo>& trackInfo);
    ~RtmpEncodeAV1();

public:
    void encode(const FrameBuffer::Ptr& frame) override;
    std::string getConfig() override;

    void setOnRtmpPacket(const std::function<void(const RtmpMessage::Ptr& msg, bool start)>& cb) override;
    void onRtmpMessage(const RtmpMessage::Ptr& msg, bool start);

private:
    bool _append = false;
    uint64_t _lastStamp = 0;
    uint64_t _msgLength = 0;
    std::shared_ptr<TrackInfo> _trackInfo;
    std::function<void(const RtmpMessage::Ptr& msg, bool start)> _onRtmpMessage;
    std::vector<FrameBuffer::Ptr> _vecFrame;
};

#endif //RtmpEncodeAV1_h