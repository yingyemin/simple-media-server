#ifndef HlsMediaSource_H
#define HlsMediaSource_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>

#include "Common/MediaSource.h"
#include "Common/DataQue.h"
#include "HlsMuxer.h"

// using namespace std;

class HlsMediaSource : public MediaSource
{
public:
    using Ptr = std::shared_ptr<HlsMediaSource>;
    using Wptr = std::weak_ptr<HlsMediaSource>;

    HlsMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr, bool muxer = false);
    ~HlsMediaSource();

public:
    void addTrack(const std::shared_ptr<TrackInfo>& track) override;
    void onFrame(const FrameBuffer::Ptr& frame) override;
    void onReady() override;
    int playerCount() override;

    std::string getM3u8(void* key);
    FrameBuffer::Ptr getTsBuffer(const std::string& key);
    void onHlsReady();

private:
    bool _muxer;

    HlsMuxer::Ptr _hlsMuxer;
};


#endif //HlsMediaSource_H
