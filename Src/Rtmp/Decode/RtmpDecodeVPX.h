#ifndef RtmpDecodeVPX_h
#define RtmpDecodeVPX_h

#include <memory>

#include "Codec/VP8Track.h"
#include "Codec/VP9Track.h"
#include "Rtmp/RtmpMessage.h"
#include "RtmpDecode.h"
#include "Codec/VP8Frame.h"
#include "Codec/VP9Frame.h"

// using namespace std;

class RtmpDecodeVPX : public RtmpDecode, public std::enable_shared_from_this<RtmpDecodeVPX> {
public:
    using Ptr = std::shared_ptr<RtmpDecodeVPX>;
    using Wptr = std::weak_ptr<RtmpDecodeVPX>;

    RtmpDecodeVPX(const std::shared_ptr<TrackInfo>& trackInfo);
    ~RtmpDecodeVPX();

public:
    FrameBuffer::Ptr createFrame();
    void decode(const RtmpMessage::Ptr& msg) override;

    void setOnFrame(const std::function<void(const FrameBuffer::Ptr& frame)>& cb) override;
    void onFrame(const FrameBuffer::Ptr& frame);

private:
    bool _first = true;
    TrackInfo::Ptr _trackInfo;
    std::function<void(const FrameBuffer::Ptr& frame)> _onFrame;
};

#endif //RtmpDecodeVPX_h