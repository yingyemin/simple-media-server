#ifndef RtmpEncodeAac_h
#define RtmpEncodeAac_h

#include <memory>

#include "Rtmp/RtmpMessage.h"
#include "Common/Frame.h"
#include "RtmpEncode.h"

// using namespace std;

class RtmpEncodeAac : public RtmpEncode, public std::enable_shared_from_this<RtmpEncodeAac> {
public:
    using Ptr = std::shared_ptr<RtmpEncodeAac>;
    using Wptr = std::weak_ptr<RtmpEncodeAac>;

    RtmpEncodeAac(const std::shared_ptr<TrackInfo>& trackInfo);
    ~RtmpEncodeAac();

public:
    void encode(const FrameBuffer::Ptr& frame) override;
    std::string getConfig() override;

    void setOnRtmpPacket(const std::function<void(const RtmpMessage::Ptr& msg, bool start)>& cb) override;
    void onRtmpMessage(const RtmpMessage::Ptr& msg, bool start);

private:
    int _audioFlag = 0;
    std::shared_ptr<TrackInfo> _trackInfo;
    std::function<void(const RtmpMessage::Ptr& msg, bool start)> _onRtmpMessage;
};

#endif //RtmpEncodeH265_h