#ifndef RtmpEncodeH264_h
#define RtmpEncodeH264_h

#include <memory>
#include <vector>

#include "Rtmp/RtmpMessage.h"
#include "Common/Frame.h"
#include "RtmpEncode.h"

using namespace std;

class RtmpEncodeH264 : public RtmpEncode, public enable_shared_from_this<RtmpEncodeH264> {
public:
    using Ptr = shared_ptr<RtmpEncodeH264>;
    using Wptr = weak_ptr<RtmpEncodeH264>;

    RtmpEncodeH264(const shared_ptr<TrackInfo>& trackInfo);
    ~RtmpEncodeH264();

public:
    void encode(const FrameBuffer::Ptr& frame) override;
    string getConfig() override;

    void setOnRtmpPacket(const function<void(const RtmpMessage::Ptr& msg, bool start)>& cb) override;
    void onRtmpMessage(const RtmpMessage::Ptr& msg, bool start);

private:
    bool _append = false;
    uint64_t _lastStamp = 0;
    uint64_t _msgLength = 0;
    shared_ptr<TrackInfo> _trackInfo;
    function<void(const RtmpMessage::Ptr& msg, bool start)> _onRtmpMessage;
    vector<FrameBuffer::Ptr> _vecFrame;
};

#endif //RtmpEncodeH264_h