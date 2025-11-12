#ifndef VP8Frame_H
#define VP8Frame_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Common/Frame.h"

// using namespace std;

enum VP8NalType
{
    VP8_KEYFRAME = 1,
    VP8_FRAME = 2,
};

class VP8Frame : public FrameBuffer
{
public:
    using Ptr = std::shared_ptr<VP8Frame>;

    VP8Frame()
    {
        _codec = "vp8";
    }
    
    VP8Frame(const VP8Frame::Ptr& frame);

    bool isNewNalu() override;

    bool keyFrame() const override;

    bool metaFrame() const override
    {
        return keyFrame();
    }

    bool startFrame() const override
    {
        return keyFrame();
    }

    uint8_t getNalType() override
    {
        if (keyFrame()) {
            return VP8_KEYFRAME;
        } else {
            return VP8_FRAME;
        }
    }

    bool isNonPicNalu() override
    {
        return false;
    }

    static uint8_t getNalType(uint8_t* nalByte, int len);

    void split(const std::function<void(const FrameBuffer::Ptr& frame)>& cb) override;
    static FrameBuffer::Ptr createFrame(int startSize, int index, bool addStart);
    
    static void registerFrame();

public:
    bool _keyframe = false;
};


#endif //VP8Frame_H