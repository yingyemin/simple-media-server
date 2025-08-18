/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef VIDEOSTACK_TRANSCODE_H
#define VIDEOSTACK_TRANSCODE_H

#if defined(ENABLE_FFMPEG)

#include "Util/TimeClock.h"
#include "Common/Track.h"
#include "Common/Frame.h"

#include <mutex>
#include <list>
#include <thread>
#include <condition_variable>

#include "TranscodeAudio.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/pixdesc.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/imgutils.h"
#include "libavformat/avformat.h"
#ifdef __cplusplus
}
#endif

class EncodeOption
{
public:
    int gop = 0;
    int width = 0;
    int height = 0;
};

// class FFmpegFrame {
// public:
//     using Ptr = std::shared_ptr<FFmpegFrame>;

//     FFmpegFrame(std::shared_ptr<AVFrame> frame = nullptr);
//     ~FFmpegFrame();

//     AVFrame *get() const;
//     void fillPicture(AVPixelFormat target_format, int target_width, int target_height);

// private:
//     char *_data = nullptr;
//     std::shared_ptr<AVFrame> _frame;
// };

class FFmpegSwr {
public:
    using Ptr = std::shared_ptr<FFmpegSwr>;

    FFmpegSwr(AVSampleFormat output, AVChannelLayout channel_layout, int samplerate);
    ~FFmpegSwr();
    FFmpegFrame::Ptr inputFrame(const FFmpegFrame::Ptr &frame);

private:
    // int _target_channels;
    AVChannelLayout _target_channel_layout;
    int _target_samplerate;
    AVSampleFormat _target_format;
    SwrContext *_ctx = nullptr;
};

class TaskManager {
public:
    virtual ~TaskManager();

    void setMaxTaskSize(size_t size);
    void stopThread(bool drop_task);

protected:
    void startThread(const std::string &name);
    bool addEncodeTask(std::function<void()> task);
    bool addDecodeTask(bool key_frame, std::function<void()> task);
    bool isEnabled() const;

private:
    void onThreadRun(const std::string &name);

private:
    class ThreadExitException : public std::runtime_error {
    public:
        ThreadExitException() : std::runtime_error("exit") {}
    };

private:
    bool _decode_drop_start = false;
    bool _exit = false;
    size_t _max_task = 30;
    std::mutex _task_mtx;
    std::condition_variable _task_cv;
    std::list<std::function<void()> > _task;
    std::shared_ptr<std::thread> _thread;
};

class FfmpegFormat {
public:
    using Ptr = std::shared_ptr<FfmpegFormat>;

    FfmpegFormat(const std::string& filename);
    ~FfmpegFormat();

public:
    bool open();
    FrameBuffer::Ptr readPacket();
    AVCodecContext* getVideoCodecContext() { return _videoCodecContext; }
    AVCodecContext* getAudioCodecContext() { return _audioCodecContext; }

private:
    bool open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type);

private:
    int _videoStreamIndex = -1;
    int _audioStreamIndex = -1;
    std::string _filename;
    AVFormatContext* _formatContext = nullptr;
    AVStream* _videoStream = nullptr;
    AVStream* _audioStream = nullptr;
    AVCodecContext* _videoCodecContext = nullptr;
    AVCodecContext* _audioCodecContext = nullptr;
    AVPacket* _packet = nullptr;
    AVFrame* _frame = nullptr;
};

class FFmpegDecoder : public TaskManager {
public:
    using Ptr = std::shared_ptr<FFmpegDecoder>;
    using onDec = std::function<void(const FFmpegFrame::Ptr &)>;

    FFmpegDecoder(const TrackInfo::Ptr &track, int thread_num = 2, const std::vector<std::string> &codec_name = {});
    FFmpegDecoder(const FfmpegFormat::Ptr& format);
    ~FFmpegDecoder() override;

    bool inputFrame(const FrameBuffer::Ptr &frame, bool live, bool async, bool enable_merge = true);
    void setOnDecode(onDec cb);
    void flush();
    const AVCodecContext *getContext() const;

private:
    void onDecode(const FFmpegFrame::Ptr &frame);
    bool inputFrame_l(const FrameBuffer::Ptr &frame, bool live, bool enable_merge);
    bool decodeFrame(const char *data, size_t size, uint64_t dts, uint64_t pts, bool live, bool key_frame);

private:
    // default merge frame
    bool _do_merger = true;
    TimeClock _ticker;
    onDec _cb;
    FfmpegFormat::Ptr _format;
    std::shared_ptr<AVCodecContext> _context;
    // FrameMerger _merger{FrameMerger::h264_prefix};
};

class FFmpegSws {
public:
    using Ptr = std::shared_ptr<FFmpegSws>;

    FFmpegSws(AVPixelFormat output, int width, int height);
    ~FFmpegSws();
    FFmpegFrame::Ptr inputFrame(const FFmpegFrame::Ptr &frame);
    int inputFrame(const FFmpegFrame::Ptr &frame, uint8_t *data);

private:
    FFmpegFrame::Ptr inputFrame(const FFmpegFrame::Ptr &frame, int &ret, uint8_t *data);

private:
    int _target_width = 0;
    int _target_height = 0;
    int _src_width = 0;
    int _src_height = 0;
    SwsContext *_ctx = nullptr;
    AVPixelFormat _src_format = AV_PIX_FMT_NONE;
    AVPixelFormat _target_format = AV_PIX_FMT_NONE;
};

class FFmpegEncoder : public std::enable_shared_from_this<FFmpegEncoder>{
public:
    //typedef std::shared_ptr<FFmpegEncoder> Ptr;
    using Ptr = std::shared_ptr<FFmpegEncoder>;
    using onEnc = std::function<void(const FrameBuffer::Ptr &)>;

    //FFmpegEncoderCustom(const Track::Ptr &track, int thread_num = 2);
    FFmpegEncoder(const TrackInfo::Ptr &track, const EncodeOption& option);
    ~FFmpegEncoder();

    void flush();
    std::string getCodecId() const { return _codecId; }
    const AVCodecContext *getContext() const { return _context.get(); }

    void setOnEncode(onEnc cb) { _cb = std::move(cb); }
    bool inputFrame(const FFmpegFrame::Ptr &frame, bool async);

private:
    bool inputFrame_l(FFmpegFrame::Ptr frame);
    bool encodeFrame(AVFrame *frame);
    void onEncode(AVPacket *packet);
    bool openVideoCodec(int width, int height, int bitrate, const AVCodec *codec);
    bool openAudioCodec(int samplerate, int channel, int bitrate, const AVCodec *codec);

private:
    uint64_t _pts = 0;
    // WorkLoop::Ptr _ffmpegencodePool;
    TrackInfo::Ptr _track;
    onEnc _cb;
    std::string _codecId;
    EncodeOption _option;
    const AVCodec *_codec = nullptr;
    AVDictionary *_dict = nullptr;
    std::shared_ptr<AVCodecContext> _context;

    std::shared_ptr<FFmpegSws> _sws;
    std::unique_ptr<FFmpegSwr> _swr;
    // std::unique_ptr<FFmpegAudioFifo> _fifo;
    bool var_frame_size = false;
};

#endif// ENABLE_FFMPEG
#endif //VIDEOSTACK_TRANSCODE_H
