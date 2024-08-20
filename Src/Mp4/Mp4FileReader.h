#ifndef Mp4FileReader_H
#define Mp4FileReader_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Mp4Demuxer.h"
#include "Util/File.h"

using namespace std;

class Mp4FileReader : public MP4Demuxer
{
public:
    using Ptr = shared_ptr<Mp4FileReader>;

    Mp4FileReader(const string& filepath);
    ~Mp4FileReader();

public:
    void write(const char* data, int size) override;
    void read(char* data, int size) override;
    void seek(uint64_t offset) override;
    size_t tell() override;
    void onFrame(const StreamBuffer::Ptr& frame, int trackIndex, int pts, int dts, bool keyframe) override;
    void onTrackInfo(const TrackInfo::Ptr& trackInfo) override;
    void onReady() override;

    bool open();

public:
    void setOnFrame(const std::function<void (const FrameBuffer::Ptr &frame)>& cb);
    void setOnReady(const function<void()>& cb);
    void setOnTrackInfo(const std::function<void (const TrackInfo::Ptr &trackInfo)> &cb);

private:
    string _filepath;
    File _file;

    std::function<void (const FrameBuffer::Ptr &frame)> _onFrame;
    function<void()> _onReady;
    std::function<void (const TrackInfo::Ptr &trackInfo)> _onTrackInfo;

    unordered_map<int, shared_ptr<TrackInfo>> _mapTrackInfo;
};


#endif //Mp4FileReader_H
