#ifndef RtmpEncodeH266_h
#define RtmpEncodeH266_h

#include <memory>

#include "Rtmp/RtmpMessage.h"
#include "Common/Frame.h"
#include "RtmpEncode.h"

using namespace std;

class RtmpEncodeH266 : public RtmpEncode, public enable_shared_from_this<RtmpEncodeH266> {
public:
    using Ptr = shared_ptr<RtmpEncodeH266>;
    using Wptr = weak_ptr<RtmpEncodeH266>;

    RtmpEncodeH266(const shared_ptr<TrackInfo>& trackInfo);
    ~RtmpEncodeH266();

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

#endif //RtmpEncodeH266_h
