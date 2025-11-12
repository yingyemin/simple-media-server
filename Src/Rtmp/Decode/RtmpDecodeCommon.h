#ifndef RtmpDecodeCommon_h
#define RtmpDecodeCommon_h

#include <memory>

#include "Rtmp/RtmpMessage.h"
#include "Common/Track.h"
#include "Common/Frame.h"
#include "RtmpDecode.h"

// using namespace std;

class RtmpDecodeCommon : public RtmpDecode, public std::enable_shared_from_this<RtmpDecodeCommon> {
public:
    using Ptr = std::shared_ptr<RtmpDecodeCommon>;
    using Wptr = std::weak_ptr<RtmpDecodeCommon>;

    RtmpDecodeCommon(const std::shared_ptr<TrackInfo>& trackInfo);
    ~RtmpDecodeCommon();

public:
    void decode(const RtmpMessage::Ptr& msg) override;

    void setOnFrame(const std::function<void(const FrameBuffer::Ptr& frame)>& cb) override;
    void onFrame(const FrameBuffer::Ptr& frame);

private:
    bool _first = true;
    TrackInfo::Ptr _trackInfo;
    std::function<void(const FrameBuffer::Ptr& frame)> _onFrame;
};

#endif //RtmpDecodeCommon_h