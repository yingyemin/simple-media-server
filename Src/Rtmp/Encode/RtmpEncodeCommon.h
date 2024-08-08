#ifndef RtmpEncodeCommon_h
#define RtmpEncodeCommon_h

#include <memory>

#include "Rtmp/RtmpMessage.h"
#include "Common/Frame.h"
#include "RtmpEncode.h"

using namespace std;

class RtmpEncodeCommon : public RtmpEncode, public enable_shared_from_this<RtmpEncodeCommon> {
public:
    using Ptr = shared_ptr<RtmpEncodeCommon>;
    using Wptr = weak_ptr<RtmpEncodeCommon>;

    RtmpEncodeCommon(const shared_ptr<TrackInfo>& trackInfo);
    ~RtmpEncodeCommon();

public:
    void encode(const FrameBuffer::Ptr& frame) override;
    string getConfig() override;

    void setOnRtmpPacket(const function<void(const RtmpMessage::Ptr& msg, bool start)>& cb) override;
    void onRtmpMessage(const RtmpMessage::Ptr& msg, bool start);

private:
    int _audioFlag = 0;
    shared_ptr<TrackInfo> _trackInfo;
    function<void(const RtmpMessage::Ptr& msg, bool start)> _onRtmpMessage;
};

#endif //RtmpEncodeH265_h