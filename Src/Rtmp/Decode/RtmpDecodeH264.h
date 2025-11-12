#ifndef RtmpDecodeH264_h
#define RtmpDecodeH264_h

#include <memory>

#include "Codec/H264Track.h"
#include "Rtmp/RtmpMessage.h"
#include "RtmpDecode.h"
#include "Codec/H264Frame.h"

// using namespace std;

class RtmpDecodeH264 : public RtmpDecode, public std::enable_shared_from_this<RtmpDecodeH264> {
public:
    using Ptr = std::shared_ptr<RtmpDecodeH264>;
    using Wptr = std::weak_ptr<RtmpDecodeH264>;

    RtmpDecodeH264(const std::shared_ptr<TrackInfo>& trackInfo);
    ~RtmpDecodeH264();

public:
    H264Frame::Ptr createFrame();
    void decode(const RtmpMessage::Ptr& msg) override;

    void setOnFrame(const std::function<void(const FrameBuffer::Ptr& frame)>& cb) override;
    void onFrame(const FrameBuffer::Ptr& frame);

private:
    bool _first = true;
    H264Track::Ptr _trackInfo;
    std::function<void(const FrameBuffer::Ptr& frame)> _onFrame;
};

#endif //RtmpDecodeH264_h