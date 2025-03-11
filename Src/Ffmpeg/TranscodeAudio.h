#ifndef TransCodeAudio_H
#define TransCodeAudio_H

#ifdef ENABLE_FFMPEG

#include <memory>

extern "C" {
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/pixdesc.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/imgutils.h"
}

#include "Common/Track.h"
#include "Common/Frame.h"
#include "Util/TimeClock.h"

class FFmpegFrame {
public:
    using Ptr = std::shared_ptr<FFmpegFrame>;

    FFmpegFrame(std::shared_ptr<AVFrame> frame = nullptr);
    ~FFmpegFrame();

    AVFrame *get() const;
    void fillPicture(AVPixelFormat target_format, int target_width, int target_height);

private:
    char *_data = nullptr;
    std::shared_ptr<AVFrame> _frame;
};

class AudioSwr {
public:
    using Ptr = std::shared_ptr<AudioSwr>;

    AudioSwr(AVSampleFormat output, AVChannelLayout channel_layout, int samplerate);
    ~AudioSwr();
    FFmpegFrame::Ptr inputFrame(const FFmpegFrame::Ptr &frame);

private:
    // int _target_channels;
    AVChannelLayout _target_channel_layout;
    int _target_samplerate;
    AVSampleFormat _target_format;
    SwrContext *_ctx = nullptr;
};

class AudioFifo {
public:
    AudioFifo() = default;
    ~AudioFifo();

    bool write(const AVFrame *frame);
    bool read(AVFrame *frame, int sample_size);
    int size() const;

private:
    int _channels = 0;
    int _samplerate = 0;
    double _tsp = 0;
    double _timebase = 0;
    AVAudioFifo *_fifo = nullptr;
    AVSampleFormat _format = AV_SAMPLE_FMT_NONE;
};

//class AudioDecoder : public TaskManager {
class AudioDecoder : public std::enable_shared_from_this<AudioDecoder>{
public:
    //typedef std::shared_ptr<AudioDecoder> Ptr;
    using Ptr = std::shared_ptr<AudioDecoder>;

    //AudioDecoder(const Track::Ptr &track, int thread_num = 2);
    AudioDecoder(const TrackInfo::Ptr &track);
    //~AudioDecoder() override;
    ~AudioDecoder();

    bool inputFrame(const FrameBuffer::Ptr &frame, bool live, bool async, bool enable_merge = true);
    void setOnDecode(const std::function<void(const FFmpegFrame::Ptr &)>& cb);
    void flush();

private:
    void onDecode(const FFmpegFrame::Ptr &frame);

private:
    bool _do_merger = false;
    std::function<void(const FFmpegFrame::Ptr &)> _onDecode;
    std::shared_ptr<AVCodecContext> _context;
    TimeClock _timeClock;
    //FrameMerger _merger{FrameMerger::h264_prefix};
    // FrameMerger _merger;
};

class AudioEncoder : public std::enable_shared_from_this<AudioEncoder>
{
public:
    using Ptr = std::shared_ptr<AudioEncoder>;

    AudioEncoder(const TrackInfo::Ptr &track);
    ~AudioEncoder();

    void flush();

    void setOnPacket(const std::function<void(const FrameBuffer::Ptr &)>& cb) { _onPakcet = std::move(cb); }
    bool inputFrame(const FFmpegFrame::Ptr &frame, bool async);

private:
    bool encode(AVFrame *frame);
    void onPacket(AVPacket *packet);
    bool initCodec(int samplerate, int channel, int bitrate, const AVCodec *codec);

private:
    uint64_t _sampleNum = 0;
    std::function<void(const FrameBuffer::Ptr &)> _onPakcet;
    std::string _codecId;
    const AVCodec *_codec = nullptr;
    //AVDictionary *_dict = nullptr;
    std::shared_ptr<AVCodecContext> _context;

    std::unique_ptr<AudioSwr> _swr;
    std::unique_ptr<AudioFifo> _fifo;
    TrackInfo::Ptr _trackInfo;
    bool var_frame_size = false;
};

#endif

#endif