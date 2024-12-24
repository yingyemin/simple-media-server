#ifndef RtmpEncodeAV1_h
#define RtmpEncodeAV1_h

#include <memory>

#include "Rtmp/RtmpMessage.h"
#include "Common/Frame.h"
#include "RtmpEncode.h"

using namespace std;

class RtmpEncodeAV1 : public RtmpEncode, public enable_shared_from_this<RtmpEncodeAV1> {
public:
    using Ptr = shared_ptr<RtmpEncodeAV1>;
    using Wptr = weak_ptr<RtmpEncodeAV1>;

    RtmpEncodeAV1(const shared_ptr<TrackInfo>& trackInfo);
    ~RtmpEncodeAV1();

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

#endif //RtmpEncodeAV1_h