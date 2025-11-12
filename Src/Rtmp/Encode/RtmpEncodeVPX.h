#ifndef RtmpEncodeVPX_h
#define RtmpEncodeVPX_h

#include <memory>

#include "Rtmp/RtmpMessage.h"
#include "Common/Frame.h"
#include "RtmpEncode.h"

// using namespace std;

class RtmpEncodeVPX : public RtmpEncode, public std::enable_shared_from_this<RtmpEncodeVPX> {
public:
    using Ptr = std::shared_ptr<RtmpEncodeVPX>;
    using Wptr = std::weak_ptr<RtmpEncodeVPX>;

    RtmpEncodeVPX(const std::shared_ptr<TrackInfo>& trackInfo);
    ~RtmpEncodeVPX();

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

#endif //RtmpEncodeVPX_h