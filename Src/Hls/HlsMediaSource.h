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

using namespace std;

class HlsMediaSource : public MediaSource
{
public:
    using Ptr = shared_ptr<HlsMediaSource>;
    using Wptr = weak_ptr<HlsMediaSource>;

    HlsMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr, bool muxer = false);
    ~HlsMediaSource();

public:
    void addTrack(const shared_ptr<TrackInfo>& track) override;
    void onFrame(const FrameBuffer::Ptr& frame) override;
    void onReady() override;

    string getM3u8(void* key);
    FrameBuffer::Ptr getTsBuffer(const string& key);
    void onHlsReady();

private:
    bool _muxer;

    HlsMuxer::Ptr _hlsMuxer;
};


#endif //HlsMediaSource_H
