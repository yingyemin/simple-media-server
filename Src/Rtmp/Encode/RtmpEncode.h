#ifndef RtmpEncode_H
#define RtmpEncode_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include "Rtmp/RtmpMessage.h"
#include "Common/Frame.h"
#include "Common/Track.h"

// using namespace std;

class RtmpEncode // : public enable_shared_from_this<RtmpEncode>
{
public:
    using Ptr = std::shared_ptr<RtmpEncode>;

    static RtmpEncode::Ptr create(const std::shared_ptr<TrackInfo>& trackInfo);

    virtual std::string getConfig() {return "";}
    virtual void setOnRtmpPacket(const std::function<void(const RtmpMessage::Ptr& packet, bool start)>& cb) = 0;
    virtual void encode(const FrameBuffer::Ptr& frame) = 0;
    void setEnhanced(bool enhanced) {_enhanced = enhanced;}
    void setFastPts(bool enabled) {_enableFastPts = enabled;}

protected:
    bool _enhanced = false;
    bool _enableFastPts = false;
    float _ptsScale = 0.95;
};


#endif //RtpEncoder_H
