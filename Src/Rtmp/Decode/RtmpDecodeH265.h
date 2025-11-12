#ifndef RtmpDecodeH265_h
#define RtmpDecodeH265_h

#include <memory>

#include "Codec/H265Track.h"
#include "Rtmp/RtmpMessage.h"
#include "RtmpDecode.h"
#include "Codec/H265Frame.h"

// using namespace std;

class RtmpDecodeH265 : public RtmpDecode, public std::enable_shared_from_this<RtmpDecodeH265> {
public:
    using Ptr = std::shared_ptr<RtmpDecodeH265>;
    using Wptr = std::weak_ptr<RtmpDecodeH265>;

    RtmpDecodeH265(const std::shared_ptr<TrackInfo>& trackInfo);
    ~RtmpDecodeH265();

public:
    H265Frame::Ptr createFrame();
    void decode(const RtmpMessage::Ptr& msg) override;

    void setOnFrame(const std::function<void(const FrameBuffer::Ptr& frame)>& cb) override;
    void onFrame(const FrameBuffer::Ptr& frame);

private:
    bool _first = true;
    H265Track::Ptr _trackInfo;
    std::function<void(const FrameBuffer::Ptr& frame)> _onFrame;
};

#endif //RtmpDecodeH265_h