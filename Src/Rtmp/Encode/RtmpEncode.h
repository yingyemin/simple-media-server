#ifndef RtmpEncode_H
#define RtmpEncode_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Rtmp/RtmpMessage.h"
#include "Common/Frame.h"
#include "Common/Track.h"

using namespace std;

class RtmpEncode // : public enable_shared_from_this<RtmpEncode>
{
public:
    using Ptr = shared_ptr<RtmpEncode>;

    static RtmpEncode::Ptr create(const shared_ptr<TrackInfo>& trackInfo);

    virtual string getConfig() {return "";}
    virtual void setOnRtmpPacket(const function<void(const RtmpMessage::Ptr& packet, bool start)>& cb) = 0;
    virtual void encode(const FrameBuffer::Ptr& frame) = 0;
    void setEnhanced(bool enhanced) {_enhanced = enhanced;}

protected:
    bool _enhanced = false;
};


#endif //RtpEncoder_H
