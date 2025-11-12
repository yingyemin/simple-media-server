#ifndef RtmpDecodeAac_h
#define RtmpDecodeAac_h

#include <memory>

#include "Rtmp/RtmpMessage.h"
#include "Codec/AacTrack.h"
#include "Common/Frame.h"
#include "RtmpDecode.h"

// using namespace std;

class RtmpDecodeAac : public RtmpDecode, public std::enable_shared_from_this<RtmpDecodeAac> {
public:
    using Ptr = std::shared_ptr<RtmpDecodeAac>;
    using Wptr = std::weak_ptr<RtmpDecodeAac>;

    RtmpDecodeAac(const std::shared_ptr<TrackInfo>& trackInfo);
    ~RtmpDecodeAac();

public:
    void decode(const RtmpMessage::Ptr& msg) override;

    void setOnFrame(const std::function<void(const FrameBuffer::Ptr& frame)>& cb) override;
    void onFrame(const FrameBuffer::Ptr& frame);

private:
    bool _first = true;
    std::string _aacConfig;
    AacTrack::Ptr _trackInfo;
    std::function<void(const FrameBuffer::Ptr& frame)> _onFrame;
};

#endif //RtmpDecodeAac_h