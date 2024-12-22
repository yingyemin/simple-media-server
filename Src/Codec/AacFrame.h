#ifndef AacFrame_H
#define AacFrame_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Common/Frame.h"

using namespace std;


class AacFrame : public FrameBuffer
{
public:
    using Ptr = shared_ptr<AacFrame>;

    AacFrame()
    {
        _codec = "aac";
    }

    static FrameBuffer::Ptr getMuteForAdts();
    static StreamBuffer::Ptr getMuteForFlv();
    static FrameBuffer::Ptr createFrame(int startSize, int index, bool addStart);
    static void registerFrame();
};


#endif //AacFrame_H
