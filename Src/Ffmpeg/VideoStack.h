#ifndef VIDEOSTACK_VIDEOSTACK_H
#define VIDEOSTACK_VIDEOSTACK_H

#if defined(ENABLE_FFMPEG)
#include "Transcode.h"
#include <mutex>
#include "EventPoller/EventLoop.h"
#include "Rtsp/RtspClient.h"
#include "Common/FrameMediaSource.h"
#include "Common/json.hpp"
#include "WorkPoller/WorkLoopPool.h"

template<typename T> class RefWrapper {
public:
    using Ptr = std::shared_ptr<RefWrapper<T>>;

    template<typename... Args>
    explicit RefWrapper(Args&&... args) : _rc(0), _entity(std::forward<Args>(args)...) {}

    T acquire() {
        ++_rc;
        return _entity;
    }

    bool dispose() { return --_rc <= 0; }

private:
    std::atomic<int> _rc;
    T _entity;
};

class Channel;

struct Param {
    using Ptr = std::shared_ptr<Param>;

    int posX = 0;
    int posY = 0;
    int width = 0;
    int height = 0;
    AVPixelFormat pixfmt = AV_PIX_FMT_YUV420P;
    std::string id{};
    std::string bgImg{};

    // runtime
    std::weak_ptr<Channel> weak_chn;
    std::weak_ptr<FFmpegFrame> weak_buf;

    ~Param();
};

using Params = std::shared_ptr<std::vector<Param::Ptr>>;

class Channel : public std::enable_shared_from_this<Channel> {
public:
    using Ptr = std::shared_ptr<Channel>;

    Channel(const std::string& id, int width, int height, AVPixelFormat pixfmt);

    void addParam(const std::weak_ptr<Param>& p);

    void onFrame(const FFmpegFrame::Ptr& frame);

    void fillBuffer(const Param::Ptr& p);

protected:
    void forEachParam(const std::function<void(const Param::Ptr&)>& func);

    void copyData(const FFmpegFrame::Ptr& buf, const Param::Ptr& p);

private:
    std::string _id;
    int _width;
    int _height;
    AVPixelFormat _pixfmt;
    WorkLoop::Ptr _workLoop;

    FFmpegFrame::Ptr _tmp;

    std::recursive_mutex _mx;
    std::vector<std::weak_ptr<Param>> _params;

    FFmpegSws::Ptr _sws;
    EventLoop::Ptr _loop;
};

class StackPlayer : public std::enable_shared_from_this<StackPlayer> {
public:
    using Ptr = std::shared_ptr<StackPlayer>;

    StackPlayer(const std::string& url) : _url(url) {}

    void addChannel(const std::weak_ptr<Channel>& chn);

    void play();

    void onFrame(const FFmpegFrame::Ptr& frame);

    void onDisconnect();

    void onOriginFrameSource(const MediaSource::Ptr &src);

protected:
    void rePlay(const std::string& url);

private:
    std::string _url;

    // 用于断线重连  [AUTO-TRANSLATED:18fd242a]
    // Used for disconnection and reconnection
    // toolkit::Timer::Ptr _timer;
    int _failedCount = 0;
    FFmpegDecoder::Ptr _videoDecoder;
    FFmpegDecoder::Ptr _audioDecoder;
    FrameMediaSource::Wptr _originSource;
    WorkLoop::Ptr _workLoop;
    MediaSource::FrameRingType::DataQueReaderT::Ptr _playReader;
    FFmpegFrame::Ptr _frame;

    std::recursive_mutex _mx;
    std::vector<std::weak_ptr<Channel>> _channels;
};

class VideoStack {
public:
    using Ptr = std::shared_ptr<VideoStack>;

    VideoStack(const std::string& url, int width = 1920, int height = 1080,
               AVPixelFormat pixfmt = AV_PIX_FMT_YUV420P, float fps = 25.0,
               int bitRate = 2 * 1024 * 1024);

    ~VideoStack();

    void setParam(const Params& params);
    void setBgImg(const std::string& bgImg);

    void start();

protected:
    void initBgColor();

public:
    std::string _bgImg;
    Params _params;

    FFmpegFrame::Ptr _buffer;

private:
    std::string _id;
    int _width = 0;
    int _height = 0;
    int _gop = 50;
    AVPixelFormat _pixfmt = AV_PIX_FMT_YUV420P;
    float _fps = 25.0;
    int _bitRate = 2 * 1024 * 1024;

    FFmpegEncoder::Ptr _encoder;
    FrameMediaSource::Ptr _frameSrc;
    TrackInfo::Ptr _videoTrackInfo;
    TrackInfo::Ptr _audioTrackInfo;

    bool _isExit;

    std::thread _thread;
};

class VideoStackManager {
public:
    // 创建拼接流  [AUTO-TRANSLATED:ebb3a8ec]
    // Create a concatenated stream
    int startVideoStack(const nlohmann::json& param);
    int startCustomVideoStack(const std::string& id, int width, int height, const std::string& bgImg, const Params& params);

    // 停止拼接流  [AUTO-TRANSLATED:a46f341f]
    // Stop the concatenated stream
    int stopVideoStack(const std::string& id);

    // 可以在不断流的情况下，修改拼接流的配置(实现切换拼接屏内容)  [AUTO-TRANSLATED:f9b59b6b]
    // You can modify the configuration of the concatenated stream (to switch the content of the concatenated screen) without stopping the stream
    int resetVideoStack(const nlohmann::json& param);

public:
    static VideoStackManager& Instance();

    Channel::Ptr getChannel(const std::string& id, int width, int height, AVPixelFormat pixfmt);

    void unrefChannel(const std::string& id, int width, int height, AVPixelFormat pixfmt);

    bool loadBgImg(const std::string& path);

    void clear();

    FFmpegFrame::Ptr getBgImg();

protected:
    Params parseParams(const nlohmann::json& param, std::string& id, int& width, int& height);

protected:
    Channel::Ptr createChannel(const std::string& id, int width, int height, AVPixelFormat pixfmt);

    StackPlayer::Ptr createPlayer(const std::string& id);

private:
    FFmpegFrame::Ptr _bgImg;

private:
    std::recursive_mutex _mx;

    std::unordered_map<std::string, VideoStack::Ptr> _stackMap;

    std::unordered_map<std::string, RefWrapper<Channel::Ptr>::Ptr> _channelMap;
    
    std::unordered_map<std::string, RefWrapper<StackPlayer::Ptr>::Ptr> _playerMap;
};

#endif

#endif //VIDEOSTACK_VIDEOSTACK_H
