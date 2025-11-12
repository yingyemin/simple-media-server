#ifndef Fmp4Demuxer_H
#define Fmp4Demuxer_H

#include "Common/Frame.h"
#include "Common/Track.h"
#include "Mp4Box.h"
#include "Mp4Demuxer.h"

#include <memory>

// using namespace std;

class Fmp4Demuxer : public MP4Demuxer
{
public:
    using Ptr = std::shared_ptr<Fmp4Demuxer>;

    Fmp4Demuxer();
    ~Fmp4Demuxer();

public:
    void write(const char* data, int size) override;
    void read(char* data, int size) override;
    void seek(uint64_t offset) override;
    size_t tell() override;
    void onFrame(const StreamBuffer::Ptr& frame, int trackIndex, int pts, int dts, bool keyframe) override;
    void onTrackInfo(const TrackInfo::Ptr& trackInfo) override;
    void onReady() override;

public:
    void setOnFrame(const std::function<void (const FrameBuffer::Ptr &frame)>& cb);
    void setOnReady(const std::function<void()>& cb);
    void setOnTrackInfo(const std::function<void (const TrackInfo::Ptr &trackInfo)> &cb);

private:
    uint64_t _offset;
    StreamBuffer::Ptr _buffer;
    std::function<void (const FrameBuffer::Ptr &frame)> _onFrame;
    std::function<void()> _onReady;
    std::function<void (const TrackInfo::Ptr &trackInfo)> _onTrackInfo;

     std::unordered_map<int, std::shared_ptr<TrackInfo>> _mapTrackInfo;
};

#endif //Fmp4Demuxer_H

