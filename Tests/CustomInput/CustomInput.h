#include "Common/FrameMediaSource.h"
#include "EventPoller/EventLoop.h"

class CustomInput : public std::enable_shared_from_this<CustomInput>
{
public:
    CustomInput(const std::string& path);
    ~CustomInput();

public:
    bool init();
    bool addVideoTrack(const std::string& codec);
    bool addAudioTrack(const std::string& codec, int sampleRate, int channel, int sampleSize);
    bool onFrame(const uint8_t* data, int size, int64_t pts, int64_t dts, int isVideo);

private:
    void initInLoop();
    void addVideoTrackInLoop(const std::string& codec);
    void addAudioTrackInLoop(const std::string& codec, int sampleRate, int channel, int sampleSize);

private:
    std::string _path;
    std::string _videoCodec;
    std::string _audioCodec;
    UrlParser _localUrlParser;
    std::weak_ptr<FrameMediaSource> _source;
    EventLoop::Ptr _eventLoop;
};