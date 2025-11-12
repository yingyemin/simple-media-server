#ifndef AacFrame_H
#define AacFrame_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Common/Frame.h"

// using namespace std;


class AacFrame : public FrameBuffer
{
public:
    using Ptr = std::shared_ptr<AacFrame>;

    AacFrame()
    {
        _codec = "aac";
    }

    void split(const std::function<void(const FrameBuffer::Ptr& frame)>& cb) override;

    static FrameBuffer::Ptr getMuteForAdts();
    static StreamBuffer::Ptr getMuteForFlv();
    static FrameBuffer::Ptr createFrame(int startSize, int index, bool addStart);
    static void registerFrame();
};


#endif //AacFrame_H
