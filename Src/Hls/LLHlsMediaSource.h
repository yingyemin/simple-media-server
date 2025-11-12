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

// using namespace std;

class LLHlsMediaSource : public MediaSource
{
public:
    using Ptr = std::shared_ptr<LLHlsMediaSource>;
    using Wptr = std::weak_ptr<LLHlsMediaSource>;

    LLHlsMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr, bool muxer = false);
    ~LLHlsMediaSource();

public:
    void addTrack(const std::shared_ptr<TrackInfo>& track) override;
    void onFrame(const FrameBuffer::Ptr& frame) override;
    void onReady() override;

    std::string getM3u8(void* key);
    FrameBuffer::Ptr getTsBuffer(const std::string& key);
    void onHlsReady();

private:
    bool _muxer;

    LLHlsMuxer::Ptr _hlsMuxer;
};


#endif //LLHlsMediaSource_H
