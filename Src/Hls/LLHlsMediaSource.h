#ifndef LLHlsMediaSource_H
#define LLHlsMediaSource_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>

#include "Common/MediaSource.h"
#include "Common/DataQue.h"
#include "LLHlsMuxer.h"

using namespace std;

class LLHlsMediaSource : public MediaSource
{
public:
    using Ptr = shared_ptr<LLHlsMediaSource>;
    using Wptr = weak_ptr<LLHlsMediaSource>;

    LLHlsMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr, bool muxer = false);
    ~LLHlsMediaSource();

public:
    void addTrack(const shared_ptr<TrackInfo>& track) override;
    void onFrame(const FrameBuffer::Ptr& frame) override;
    void onReady() override;

    string getM3u8(void* key);
    FrameBuffer::Ptr getTsBuffer(const string& key);
    void onHlsReady();

private:
    bool _muxer;

    LLHlsMuxer::Ptr _hlsMuxer;
};


#endif //LLHlsMediaSource_H
