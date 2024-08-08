#ifndef RtmpDecode_h
#define RtmpDecode_h

#include <memory>
#include <functional>

#include "Rtmp/RtmpMessage.h"
#include "Common/Frame.h"

using namespace std;

class RtmpDecode
{
public:
    using Ptr = shared_ptr<RtmpDecode>;
    using Wptr = weak_ptr<RtmpDecode>;

    RtmpDecode();
    ~RtmpDecode();

public:
    virtual void decode(const RtmpMessage::Ptr& msg) {}

    virtual void setOnFrame(const function<void(const FrameBuffer::Ptr& frame)> cb) {}
};

#endif //RtmpDecode_h