#ifndef RtmpDecodeAV1_h
#define RtmpDecodeAV1_h

#include <memory>

#include "Codec/AV1Track.h"
#include "Rtmp/RtmpMessage.h"
#include "RtmpDecode.h"
#include "Codec/AV1Frame.h"

using namespace std;

class RtmpDecodeAV1 : public RtmpDecode, public enable_shared_from_this<RtmpDecodeAV1> {
public:
    using Ptr = shared_ptr<RtmpDecodeAV1>;
    using Wptr = weak_ptr<RtmpDecodeAV1>;

    RtmpDecodeAV1(const shared_ptr<TrackInfo>& trackInfo);
    ~RtmpDecodeAV1();

public:
    AV1Frame::Ptr createFrame();
    void decode(const RtmpMessage::Ptr& msg) override;

    void setOnFrame(const function<void(const FrameBuffer::Ptr& frame)> cb) override;
    void onFrame(const FrameBuffer::Ptr& frame);

private:
    bool _first = true;
    AV1Track::Ptr _trackInfo;
    function<void(const FrameBuffer::Ptr& frame)> _onFrame;
};

#endif //RtmpDecodeAV1_h