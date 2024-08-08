#ifndef RtmpDecodeAac_h
#define RtmpDecodeAac_h

#include <memory>

#include "Rtmp/RtmpMessage.h"
#include "Codec/AacTrack.h"
#include "Common/Frame.h"
#include "RtmpDecode.h"

using namespace std;

class RtmpDecodeAac : public RtmpDecode, public enable_shared_from_this<RtmpDecodeAac> {
public:
    using Ptr = shared_ptr<RtmpDecodeAac>;
    using Wptr = weak_ptr<RtmpDecodeAac>;

    RtmpDecodeAac(const shared_ptr<TrackInfo>& trackInfo);
    ~RtmpDecodeAac();

public:
    void decode(const RtmpMessage::Ptr& msg) override;

    void setOnFrame(const function<void(const FrameBuffer::Ptr& frame)> cb) override;
    void onFrame(const FrameBuffer::Ptr& frame);

private:
    bool _first = true;
    string _aacConfig;
    AacTrack::Ptr _trackInfo;
    function<void(const FrameBuffer::Ptr& frame)> _onFrame;
};

#endif //RtmpDecodeAac_h