#ifndef RtmpDecodeH265_h
#define RtmpDecodeH265_h

#include <memory>

#include "Codec/H265Track.h"
#include "Rtmp/RtmpMessage.h"
#include "RtmpDecode.h"
#include "Codec/H265Frame.h"

using namespace std;

class RtmpDecodeH265 : public RtmpDecode, public enable_shared_from_this<RtmpDecodeH265> {
public:
    using Ptr = shared_ptr<RtmpDecodeH265>;
    using Wptr = weak_ptr<RtmpDecodeH265>;

    RtmpDecodeH265(const shared_ptr<TrackInfo>& trackInfo);
    ~RtmpDecodeH265();

public:
    H265Frame::Ptr createFrame();
    void decode(const RtmpMessage::Ptr& msg) override;

    void setOnFrame(const function<void(const FrameBuffer::Ptr& frame)> cb) override;
    void onFrame(const FrameBuffer::Ptr& frame);

private:
    bool _first = true;
    H265Track::Ptr _trackInfo;
    function<void(const FrameBuffer::Ptr& frame)> _onFrame;
};

#endif //RtmpDecodeH265_h