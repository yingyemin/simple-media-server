#ifndef RtmpEncodeH265_h
#define RtmpEncodeH265_h

#include <memory>

#include "Rtmp/RtmpMessage.h"
#include "Common/Frame.h"
#include "RtmpEncode.h"

using namespace std;

class RtmpEncodeH265 : public RtmpEncode, public enable_shared_from_this<RtmpEncodeH265> {
public:
    using Ptr = shared_ptr<RtmpEncodeH265>;
    using Wptr = weak_ptr<RtmpEncodeH265>;

    RtmpEncodeH265(const shared_ptr<TrackInfo>& trackInfo);
    ~RtmpEncodeH265();

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

#endif //RtmpEncodeH265_h