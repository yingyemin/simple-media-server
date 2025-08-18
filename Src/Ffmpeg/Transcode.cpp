/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#if defined(ENABLE_FFMPEG)
#if !defined(_WIN32)
#include <dlfcn.h>
#endif
#include "Util/File.h"
#include "Util/String.h"
#include "Transcode.h"
#include "Common/config.h"
#include "Log/Logger.h"
#include "Util/Thread.h"
#define MAX_DELAY_SECOND 3

using namespace std;

static string ffmpeg_err(int errnum) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return errbuf;
}

static std::shared_ptr<AVPacket> alloc_av_packet() {
    auto pkt = std::shared_ptr<AVPacket>(av_packet_alloc(), [](AVPacket *pkt) {
        av_packet_free(&pkt);
    });
    pkt->data = NULL;    // packet data will be allocated by the encoder
    pkt->size = 0;
    return pkt;
}

//////////////////////////////////////////////////////////////////////////////////////////
static void on_ffmpeg_log(void *ctx, int level, const char *fmt, va_list args) {
    static bool enable_ffmpeg_log = Config::instance()->getAndListen([](const json& config) {
        enable_ffmpeg_log = Config::instance()->get("Ffmpeg", "enableFfmpegLog");
    }, "Ffmpeg", "enableFfmpegLog");
    if (!enable_ffmpeg_log) {
        return;
    }
    LogLevel lev;
    switch (level) {
        case AV_LOG_FATAL: lev = LError; break;
        case AV_LOG_ERROR: lev = LError; break;
        case AV_LOG_WARNING: lev = LWarn; break;
        case AV_LOG_INFO: lev = LInfo; break;
        case AV_LOG_VERBOSE: lev = LDebug; break;
        case AV_LOG_DEBUG: lev = LDebug; break;
        case AV_LOG_TRACE: lev = LTrace; break;
        default: lev = LTrace; break;
    }
    // LoggerWrapper::printLogV(::toolkit::getLogger(), lev, __FILE__, ctx ? av_default_item_name(ctx) : "NULL", level, fmt, args);
    logInfo << (ctx ? av_default_item_name(ctx) : "NULL");
}

static bool setupFFmpeg_l() {
    av_log_set_level(AV_LOG_TRACE);
    av_log_set_flags(AV_LOG_PRINT_LEVEL);
    av_log_set_callback(on_ffmpeg_log);
#if (LIBAVCODEC_VERSION_MAJOR < 58)
    avcodec_register_all();
#endif
    return true;
}

static void setupFFmpeg() {
    static auto flag = setupFFmpeg_l();
}

static bool checkIfSupportedNvidia_l() {
#if !defined(_WIN32)
    static bool check_nvidia_dev = Config::instance()->getAndListen([](const json& config) {
        check_nvidia_dev = Config::instance()->get("Ffmpeg", "checkNvidiaDev");
    }, "Ffmpeg", "checkNvidiaDev");
    if (!check_nvidia_dev) {
        return false;
    }
    auto so = dlopen("libnvcuvid.so.1", RTLD_LAZY);
    if (!so) {
        logWarn << "libnvcuvid.so.1加载失败:" << strerror(errno);
        return false;
    }
    dlclose(so);

    bool find_driver = false;
    File::scanDir("/dev", [&](const string &path, bool is_dir) {
        if (!is_dir && startWith(path, "/dev/nvidia")) {
            // 找到nvidia的驱动  [AUTO-TRANSLATED:5b87bf81]
            // Find the Nvidia driver
            find_driver = true;
            return false;
        }
        return true;
    }, false);

    if (!find_driver) {
        logWarn << "英伟达硬件编解码器驱动文件 /dev/nvidia* 不存在";
    }
    return find_driver;
#else
    return false;
#endif
}

static bool checkIfSupportedNvidia() {
    static auto ret = checkIfSupportedNvidia_l();
    return ret;
}

//////////////////////////////////////////////////////////////////////////////////////////

bool TaskManager::addEncodeTask(function<void()> task) {
    {
        lock_guard<mutex> lck(_task_mtx);
        _task.emplace_back(std::move(task));
        if (_task.size() > _max_task) {
            logWarn << "encoder thread task is too more, now drop frame!";
            _task.pop_front();
        }
    }
    _task_cv.notify_one();
    return true;
}

bool TaskManager::addDecodeTask(bool key_frame, function<void()> task) {
    {
        lock_guard<mutex> lck(_task_mtx);
        if (_decode_drop_start) {
            if (!key_frame) {
                logTrace << "decode thread drop frame";
                return false;
            }
            _decode_drop_start = false;
            logInfo << "decode thread stop drop frame";
        }

        _task.emplace_back(std::move(task));
        if (_task.size() > _max_task) {
            _decode_drop_start = true;
            logWarn << "decode thread start drop frame";
        }
    }
    _task_cv.notify_one();
    return true;
}

void TaskManager::setMaxTaskSize(size_t size) {
    if (size >= 3 && size <= 1000) {
        _max_task = size;
    } else {
        logError << "async task size limited to 3 ~ 1000, now size is:" << size;
    }
}

void TaskManager::startThread(const string &name) {
    _thread.reset(new thread([this, name]() {
        onThreadRun(name);
    }), [](thread *ptr) {
        ptr->join();
        delete ptr;
    });
}

void TaskManager::stopThread(bool drop_task) {
    if (!_thread) {
        return;
    }
    {
        lock_guard<mutex> lck(_task_mtx);
        if (drop_task) {
            _exit = true;
            _task.clear();
        }
        _task.emplace_back([]() {
            throw ThreadExitException();
        });
    }
    _task_cv.notify_all();
    _thread = nullptr;
}

TaskManager::~TaskManager() {
    stopThread(true);
}

bool TaskManager::isEnabled() const {
    return _thread.operator bool();
}

void TaskManager::onThreadRun(const string &name) {
    Thread::setThreadName(name.data());
    function<void()> task;
    _exit = false;
    while (!_exit) {
        std::function<void()> task;
        {
            unique_lock<mutex> lck(_task_mtx);
            _task_cv.wait(lck, [this]() { return !_task.empty() || _exit; });
            if (_task.empty()) {
                continue;
            }
            task = _task.front();
            _task.pop_front();
            lck.unlock();
        }

        try {
            // TimeTicker2(50, logTrace);
            task();
            task = nullptr;
        } catch (ThreadExitException &ex) {
            break;
        } catch (std::exception &ex) {
            logWarn << ex.what();
            continue;
        } catch (...) {
            logWarn << "catch one unknown exception";
            throw;
        }
    }
    logInfo << name << " exited!";
}

//////////////////////////////////////////////////////////////////////////////////////////

// FFmpegFrame::FFmpegFrame(std::shared_ptr<AVFrame> frame) {
//     if (frame) {
//         _frame = std::move(frame);
//     } else {
//         _frame.reset(av_frame_alloc(), [](AVFrame *ptr) {
//             av_frame_free(&ptr);
//         });
//     }
// }

// FFmpegFrame::~FFmpegFrame() {
//     if (_data) {
//         delete[] _data;
//         _data = nullptr;
//     }
// }

// AVFrame *FFmpegFrame::get() const {
//     return _frame.get();
// }

// void FFmpegFrame::fillPicture(AVPixelFormat target_format, int target_width, int target_height) {
//     assert(_data == nullptr);
//     _data = new char[av_image_get_buffer_size(target_format, target_width, target_height, 32)];
//     av_image_fill_arrays(_frame->data, _frame->linesize, (uint8_t *) _data,  target_format, target_width, target_height, 32);
// }

///////////////////////////////////////////////////////////////////////////

template<bool decoder = true>
static inline const AVCodec *getCodec_l(const char *name) {
    auto codec = decoder ? avcodec_find_decoder_by_name(name) : avcodec_find_encoder_by_name(name);
    if (codec) {
        logInfo << (decoder ? "got decoder:" : "got encoder:") << name;
    } else {
        logTrace << (decoder ? "decoder:" : "encoder:") << name << " not found";
    }
    return codec;
}

template<bool decoder = true>
static inline const AVCodec *getCodec_l(enum AVCodecID id) {
    auto codec = decoder ? avcodec_find_decoder(id) : avcodec_find_encoder(id);
    if (codec) {
        logInfo << (decoder ? "got decoder:" : "got encoder:") << avcodec_get_name(id);
    } else {
        logTrace << (decoder ? "decoder:" : "encoder:") << avcodec_get_name(id) << " not found";
    }
    return codec;
}

class CodecName {
public:
    CodecName(string name) : _codec_name(std::move(name)) {}
    CodecName(enum AVCodecID id) : _id(id) {}

    template <bool decoder>
    const AVCodec *getCodec() const {
        if (!_codec_name.empty()) {
            return getCodec_l<decoder>(_codec_name.data());
        }
        return getCodec_l<decoder>(_id);
    }

private:
    string _codec_name;
    enum AVCodecID _id;
};

template <bool decoder = true>
static inline const AVCodec *getCodec(const std::initializer_list<CodecName> &codec_list) {
    const AVCodec *ret = nullptr;
    for (int i = codec_list.size(); i >= 1; --i) {
        ret = codec_list.begin()[i - 1].getCodec<decoder>();
        if (ret) {
            return ret;
        }
    }
    return ret;
}

template<bool decoder = true>
static inline const AVCodec *getCodecByName(const std::vector<std::string> &codec_list) {
    const AVCodec *ret = nullptr;
    for (auto &codec : codec_list) {
        ret = getCodec_l<decoder>(codec.data());
        if (ret) {
            return ret;
        }
    }
    return ret;
}

FFmpegDecoder::FFmpegDecoder(const TrackInfo::Ptr &track, int thread_num, const std::vector<std::string> &codec_name) {
    setupFFmpeg();
    const AVCodec *codec = nullptr;
    const AVCodec *codec_default = nullptr;
    if (!codec_name.empty()) {
        codec = getCodecByName(codec_name);
    }
    
    do {
        if (track->codec_ == "h264") {
            codec_default = getCodec({AV_CODEC_ID_H264});
            if (codec && codec->id == AV_CODEC_ID_H264) {
                break;
            }
            if (checkIfSupportedNvidia()) {
                codec = getCodec({{"libopenh264"}, {AV_CODEC_ID_H264}, {"h264_qsv"}, {"h264_videotoolbox"}, {"h264_cuvid"}, {"h264_nvmpi"}});
            } else {
                codec = getCodec({{"libopenh264"}, {AV_CODEC_ID_H264}, {"h264_qsv"}, {"h264_videotoolbox"}, {"h264_nvmpi"}});
            }
        } else if (track->codec_ == "h265") {
            codec_default = getCodec({AV_CODEC_ID_HEVC});
            if (codec && codec->id == AV_CODEC_ID_HEVC) {
                break;
            }
            if (checkIfSupportedNvidia()) {
                codec = getCodec({{AV_CODEC_ID_HEVC}, {"hevc_qsv"}, {"hevc_videotoolbox"}, {"hevc_cuvid"}, {"hevc_nvmpi"}});
            } else {
                codec = getCodec({{AV_CODEC_ID_HEVC}, {"hevc_qsv"}, {"hevc_videotoolbox"}, {"hevc_nvmpi"}});
            }
        } else if (track->codec_ == "aac") {
            if (codec && codec->id == AV_CODEC_ID_AAC) {
                break;
            }
            codec = getCodec({AV_CODEC_ID_AAC});
        } else if (track->codec_ == "g711a") {
            if (codec && codec->id == AV_CODEC_ID_PCM_ALAW) {
                break;
            }
            codec = getCodec({AV_CODEC_ID_PCM_ALAW});
        } else if (track->codec_ == "g711u") {
            if (codec && codec->id == AV_CODEC_ID_PCM_MULAW) {
                break;
            }
            codec = getCodec({AV_CODEC_ID_PCM_MULAW});
        } else if (track->codec_ == "opus") {
            if (codec && codec->id == AV_CODEC_ID_OPUS) {
                break;
            }
            codec = getCodec({AV_CODEC_ID_OPUS});
        } else if (track->codec_ == "jpeg") {
            if (codec && codec->id == AV_CODEC_ID_MJPEG) {
                break;
            }
            codec = getCodec({AV_CODEC_ID_MJPEG});
        } else if (track->codec_ == "vp8") {
            if (codec && codec->id == AV_CODEC_ID_VP8) {
                break;
            }
            codec = getCodec({AV_CODEC_ID_VP8});
        } else if (track->codec_ == "vp9") {
            if (codec && codec->id == AV_CODEC_ID_VP9) {
                break;
            }
            codec = getCodec({AV_CODEC_ID_VP9});
        } else if (track->codec_ == "av1") {
            if (codec && codec->id == AV_CODEC_ID_AV1) {
                break;
            }
            codec = getCodec({AV_CODEC_ID_AV1});
        } else {
            codec = nullptr;
        }
    } while (false);

    codec = codec ? codec : codec_default;
    if (!codec) {
        throw std::runtime_error("未找到解码器");
    }

    while (true) {
        _context.reset(avcodec_alloc_context3(codec), [](AVCodecContext *ctx) {
            avcodec_free_context(&ctx);
        });

        if (!_context) {
            throw std::runtime_error("创建解码器失败");
        }

        // 保存AVFrame的引用  [AUTO-TRANSLATED:2df53d07]
        // Save the AVFrame reference
#ifdef FF_API_OLD_ENCDEC
        _context->refcounted_frames = 1;
#endif
        _context->flags |= AV_CODEC_FLAG_LOW_DELAY;
        _context->flags2 |= AV_CODEC_FLAG2_FAST;
        if (track->trackType_ == "video") {
            _context->width = track->_width;
            _context->height = track->_height;
        }

        if (track->codec_ == "g711a" || track->codec_ == "g711u") {
            // AudioTrack::Ptr audio = static_pointer_cast<AudioTrack>(track);
            //_context->channels = audio->getAudioChannel();
            _context->sample_rate = track->samplerate_;
            //_context->channel_layout = av_get_default_channel_layout(_context->channels);
            av_channel_layout_default(&_context->ch_layout, track->channel_);
        }
        AVDictionary *dict = nullptr;
        if (thread_num <= 0) {
            av_dict_set(&dict, "threads", "auto", 0);
        } else {
            av_dict_set(&dict, "threads", to_string((unsigned int)thread_num < thread::hardware_concurrency() ? thread_num : thread::hardware_concurrency()).data(), 0);
        }
        av_dict_set(&dict, "zerolatency", "1", 0);
        av_dict_set(&dict, "strict", "-2", 0);

#ifdef AV_CODEC_CAP_TRUNCATED
        if (codec->capabilities & AV_CODEC_CAP_TRUNCATED) {
            /* we do not send complete frames */
            _context->flags |= AV_CODEC_FLAG_TRUNCATED;
            _do_merger = false;
        } else {
            // 此时业务层应该需要合帧  [AUTO-TRANSLATED:8dea0fff]
            // The business layer should need to merge frames at this time
            _do_merger = true;
        }
#endif

        int ret = avcodec_open2(_context.get(), codec, &dict);
        av_dict_free(&dict);
        if (ret >= 0) {
            // 成功  [AUTO-TRANSLATED:7d878ca9]
            // Success
            logInfo << "打开解码器成功:" << codec->name;
            break;
        }

        if (codec_default && codec_default != codec) {
            // 硬件编解码器打开失败，尝试软件的  [AUTO-TRANSLATED:060200f4]
            // Hardware codec failed to open, try software codec
            logWarn << "打开解码器" << codec->name << "失败，原因是:" << ffmpeg_err(ret) << ", 再尝试打开解码器" << codec_default->name;
            codec = codec_default;
            continue;
        }
        throw std::runtime_error(string("打开解码器") + codec->name + string("失败:") + ffmpeg_err(ret));
    }
}

FFmpegDecoder::FFmpegDecoder(const FfmpegFormat::Ptr& format) {
    _format = format;
    // 因为解码器会在format里释放，这里不做释放
    _context = std::shared_ptr<AVCodecContext>(_format->getVideoCodecContext(), [](AVCodecContext *ctx) {
        // avcodec_free_context(&ctx);
    });
}

FFmpegDecoder::~FFmpegDecoder() {
    stopThread(true);
    // if (_do_merger) {
    //     _merger.flush();
    // }
    flush();
}

void FFmpegDecoder::flush() {
    while (true) {
        auto out_frame = std::make_shared<FFmpegFrame>();
        auto ret = avcodec_receive_frame(_context.get(), out_frame->get());
        if (ret == AVERROR(EAGAIN)) {
            avcodec_send_packet(_context.get(), nullptr);
            continue;
        }
        if (ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            logWarn << "avcodec_receive_frame failed:" << ffmpeg_err(ret);
            break;
        }
        onDecode(out_frame);
    }
}

const AVCodecContext *FFmpegDecoder::getContext() const {
    return _context.get();
}

bool FFmpegDecoder::inputFrame_l(const FrameBuffer::Ptr &frame, bool live, bool enable_merge) {
    // if (_do_merger && enable_merge) {
    //     return _merger.inputFrame(frame, [this, live](uint64_t dts, uint64_t pts, const Buffer::Ptr &buffer, bool have_idr) {
    //         decodeFrame(buffer->data(), buffer->size(), dts, pts, live, have_idr);
    //     });
    // }

    return decodeFrame(frame->data(), frame->size(), frame->dts(), frame->pts(), live, frame->keyFrame());
}

bool FFmpegDecoder::inputFrame(const FrameBuffer::Ptr &frame, bool live, bool async, bool enable_merge) {
    if (async && !TaskManager::isEnabled() && getContext()->codec_type == AVMEDIA_TYPE_VIDEO) {
        // 开启异步编码，且为视频，尝试启动异步解码线程  [AUTO-TRANSLATED:17a68fc6]
        // Enable asynchronous encoding, and it is video, try to start asynchronous decoding thread
        startThread("decoder thread");
    }

    if (!async || !TaskManager::isEnabled()) {
        return inputFrame_l(frame, live, enable_merge);
    }

    // auto frame_cache = Frame::getCacheAbleFrame(frame);
    return addDecodeTask(frame->keyFrame(), [this, live, frame, enable_merge]() {
        inputFrame_l(frame, live, enable_merge);
        // 此处模拟解码太慢导致的主动丢帧  [AUTO-TRANSLATED:fc8bea8a]
        // Here simulates decoding too slow, resulting in active frame dropping
        //usleep(100 * 1000);
    });
}

bool FFmpegDecoder::decodeFrame(const char *data, size_t size, uint64_t dts, uint64_t pts, bool live, bool key_frame) {
    // TimeTicker2(30, logTrace);

    auto pkt = alloc_av_packet();
    pkt->data = (uint8_t *) data;
    pkt->size = size;
    pkt->dts = dts;
    pkt->pts = pts;
    if (key_frame) {
        pkt->flags |= AV_PKT_FLAG_KEY;
    }

    auto ret = avcodec_send_packet(_context.get(), pkt.get());
    if (ret < 0) {
        if (ret != AVERROR_INVALIDDATA) {
            logWarn << "avcodec_send_packet failed:" << ffmpeg_err(ret);
        }
        return false;
    }

    while (true) {
        auto out_frame = std::make_shared<FFmpegFrame>();
        ret = avcodec_receive_frame(_context.get(), out_frame->get());
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            logWarn << "avcodec_receive_frame failed:" << ffmpeg_err(ret);
            break;
        }
        if (live && pts - out_frame->get()->pts > MAX_DELAY_SECOND * 1000 && _ticker.createToNow() > 10 * 1000) {
            // 后面的帧才忽略,防止Track无法ready  [AUTO-TRANSLATED:23f1a7c9]
            // The following frames are ignored to prevent the Track from being ready
            logWarn << "解码时，忽略" << MAX_DELAY_SECOND << "秒前的数据:" << pts << " " << out_frame->get()->pts;
            continue;
        }
        onDecode(out_frame);
    }
    return true;
}

void FFmpegDecoder::setOnDecode(FFmpegDecoder::onDec cb) {
    _cb = std::move(cb);
}

void FFmpegDecoder::onDecode(const FFmpegFrame::Ptr &frame) {
    if (_cb) {
        _cb(frame);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

FFmpegSwr::FFmpegSwr(AVSampleFormat output, AVChannelLayout channel_layout, int samplerate) {
    _target_format = output;
    // _target_channels = channel;
    // _target_channel_layout = channel_layout;
    av_channel_layout_copy(&_target_channel_layout, &channel_layout);
    _target_samplerate = samplerate;
}

FFmpegSwr::~FFmpegSwr() {
    if (_ctx) {
        swr_free(&_ctx);
    }
}

FFmpegFrame::Ptr FFmpegSwr::inputFrame(const FFmpegFrame::Ptr &frame) {
    if (frame->get()->format == _target_format &&
        // frame->get()->channels == _target_channels &&
        // frame->get()->channel_layout == (uint64_t)_target_channel_layout &&
        !av_channel_layout_compare(&frame->get()->ch_layout, &_target_channel_layout) &&
        frame->get()->sample_rate == _target_samplerate) {
        // 不转格式  [AUTO-TRANSLATED:31dc6ae1]
        // Do not convert format
        return frame;
    }
    if (!_ctx) {
        int error = swr_alloc_set_opts2(&_ctx, &_target_channel_layout, _target_format, _target_samplerate,
                                  &frame->get()->ch_layout, (AVSampleFormat) frame->get()->format,
                                  frame->get()->sample_rate, 0, nullptr);
        if (error < 0) {
            throw std::runtime_error("重采样初始化失败");
        }
        logInfo << "swr_alloc_set_opts:" << av_get_sample_fmt_name((enum AVSampleFormat) frame->get()->format) << " -> "
              << av_get_sample_fmt_name(_target_format);
    }
    if (_ctx) {
        auto out = std::make_shared<FFmpegFrame>();
        out->get()->format = _target_format;
        // out->get()->channel_layout = _target_channel_layout;
        av_channel_layout_copy(&out->get()->ch_layout, &_target_channel_layout);
        // out->get()->channels = _target_channels;
        out->get()->sample_rate = _target_samplerate;
        out->get()->pkt_dts = frame->get()->pkt_dts;
        out->get()->pts = frame->get()->pts;

        int ret = 0;
        if (0 != (ret = swr_convert_frame(_ctx, out->get(), frame->get()))) {
            logWarn << "swr_convert_frame failed:" << ffmpeg_err(ret);
            return nullptr;
        }
        return out;
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

FFmpegSws::FFmpegSws(AVPixelFormat output, int width, int height) {
    _target_format = output;
    _target_width = width;
    _target_height = height;
}

FFmpegSws::~FFmpegSws() {
    if (_ctx) {
        sws_freeContext(_ctx);
        _ctx = nullptr;
    }
}

int FFmpegSws::inputFrame(const FFmpegFrame::Ptr &frame, uint8_t *data) {
    int ret;
    inputFrame(frame, ret, data);
    return ret;
}

FFmpegFrame::Ptr FFmpegSws::inputFrame(const FFmpegFrame::Ptr &frame) {
    int ret;
    return inputFrame(frame, ret, nullptr);
}

FFmpegFrame::Ptr FFmpegSws::inputFrame(const FFmpegFrame::Ptr &frame, int &ret, uint8_t *data) {
    ret = -1;
    // TimeTicker2(30, logTrace);
    auto target_width = _target_width ? _target_width : frame->get()->width;
    auto target_height = _target_height ? _target_height : frame->get()->height;
    if (frame->get()->format == _target_format && frame->get()->width == target_width && frame->get()->height == target_height) {
        // 不转格式  [AUTO-TRANSLATED:31dc6ae1]
        // Do not convert format
        return frame;
    }
    if (_ctx && (_src_width != frame->get()->width || _src_height != frame->get()->height || _src_format != (enum AVPixelFormat) frame->get()->format)) {
        // 输入分辨率发生变化了  [AUTO-TRANSLATED:0e4ea2e8]
        // Input resolution has changed
        sws_freeContext(_ctx);
        _ctx = nullptr;
    }
    if (!_ctx) {
        _src_format = (enum AVPixelFormat) frame->get()->format;
        _src_width = frame->get()->width;
        _src_height = frame->get()->height;
        _ctx = sws_getContext(frame->get()->width, frame->get()->height, (enum AVPixelFormat) frame->get()->format, target_width, target_height, _target_format, SWS_FAST_BILINEAR, NULL, NULL, NULL);
        logInfo << "sws_getContext:" << av_get_pix_fmt_name((enum AVPixelFormat) frame->get()->format) << " -> " << av_get_pix_fmt_name(_target_format);
    }
    if (_ctx) {
        auto out = std::make_shared<FFmpegFrame>();
        if (!out->get()->data[0]) {
            if (data) {
                av_image_fill_arrays(out->get()->data, out->get()->linesize, data, _target_format, target_width, target_height, 32);
            } else {
                out->fillPicture(_target_format, target_width, target_height);
            }
        }
        if (0 >= (ret = sws_scale(_ctx, frame->get()->data, frame->get()->linesize, 0, frame->get()->height, out->get()->data, out->get()->linesize))) {
            logWarn << "sws_scale failed:" << ffmpeg_err(ret);
            return nullptr;
        }

        out->get()->format = _target_format;
        out->get()->width = target_width;
        out->get()->height = target_height;
        out->get()->pkt_dts = frame->get()->pkt_dts;
        out->get()->pts = frame->get()->pts;
        return out;
    }
    return nullptr;
}


void setupContext(AVCodecContext *_context, int bitrate, int samplerate) {
    //保存AVFrame的引用
#ifdef FF_API_OLD_ENCDEC
    _context->refcounted_frames = 1;
#endif
    _context->flags |= AV_CODEC_FLAG_LOW_DELAY;
    _context->flags2 |= AV_CODEC_FLAG2_FAST;
    _context->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    _context->time_base.num = 1;
    _context->time_base.den = 1000; // {1, 1000}
    //_context->bit_rate = bitrate;
}

FFmpegEncoder::FFmpegEncoder(const TrackInfo::Ptr &track, const EncodeOption& option) {
    //logInfo << "FFmpegEncoder构造" << endl;
    _option = option;
    _track = track;
    setupFFmpeg();
    const AVCodec *codec = nullptr;
    const AVCodec *codec_default = nullptr;
    _codecId = track->codec_;
    if (_codecId == "h264") {
        codec_default = getCodec<false>({ AV_CODEC_ID_H264 });
        if (checkIfSupportedNvidia()) {
            codec = getCodec<false>({ { "libopenh264" },
                                      { AV_CODEC_ID_H264 },
                                      { "h264_qsv" },
                                      { "h264_videotoolbox" },
                                      { "h264_cuvid" },
                                      { "h264_nvmpi" } });
        } else {
            codec = getCodec<false>({ { "libopenh264" }, { AV_CODEC_ID_H264 }, { "h264_qsv" }, { "h264_videotoolbox" }, { "h264_nvmpi" } });
        }
    } else if (_codecId == "h265") {
        codec_default = getCodec<false>({ AV_CODEC_ID_HEVC });
        if (checkIfSupportedNvidia()) {
            codec = getCodec<false>({ { AV_CODEC_ID_HEVC }, { "hevc_qsv" }, { "hevc_videotoolbox" }, { "hevc_cuvid" }, { "hevc_nvmpi" } });
        } else {
            codec = getCodec<false>({ { AV_CODEC_ID_HEVC }, { "hevc_qsv" }, { "hevc_videotoolbox" }, { "hevc_nvmpi" } });
        }
    } else if (_codecId == "aac") {
        codec = getCodec<false>({ AV_CODEC_ID_AAC });
    } else if (_codecId == "g711a") {
        codec = getCodec<false>({ AV_CODEC_ID_PCM_ALAW });
    } else if (_codecId == "g711u") {
        codec = getCodec<false>({ AV_CODEC_ID_PCM_MULAW });
    } else if (_codecId == "opus") {
        codec = getCodec<false>({ AV_CODEC_ID_OPUS });
    } else if (_codecId == "vp8") {
        codec = getCodec<false>({ AV_CODEC_ID_VP8 });
    } else if (_codecId == "vp9") {
        codec = getCodec<false>({ AV_CODEC_ID_VP9 });
    } else {
        codec = codec_default;
    }

    if (!codec) {
        throw std::runtime_error("未找到编码器");
    }

    // if (thread_num <= 0) {
        // av_dict_set(&_dict, "threads", "auto", 0);
    // } else {
        av_dict_set(&_dict, "threads", to_string(4 < thread::hardware_concurrency() ? 4 : thread::hardware_concurrency()).data(), 0);
    // }
    av_dict_set(&_dict, "zerolatency", "1", 0);
    if (strcmp(codec->name, "libx264") == 0 || strcmp(codec->name, "libx265") == 0) {
        av_dict_set(&_dict, "preset", "ultrafast", 0);
    }

    while (true) {
        bool ret = false;
        if (track->trackType_ == "video") {
            // 不设置时，仅第一个I帧前存一次sps和pps
            // 设置后，I帧钱不会存储sps和pps, 但_context->extradata会有数据，需要手动处理
            // _context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            // VideoTrack::Ptr video = static_pointer_cast<VideoTrack>(track);
            //ret = openVideoCodec(video->getVideoWidth(), video->getVideoHeight(), track->getBitRate(), codec);
            logInfo << "视频编码器" << "VideoWidth:" << track->_width << " Height:" << track->_height << " BitRate:" << track->_bitrate << endl;
            ret = openVideoCodec(track->_width, track->_height, track->_bitrate, codec);//暂定
        } else {
            // AudioTrack::Ptr audio = static_pointer_cast<AudioTrack>(track);
            ret = openAudioCodec(track->samplerate_, track->channel_, track->_bitrate, codec);
            logInfo << "音频编码器" << "AudioSampleRate:" << track->samplerate_ << " AudioChannel:" << track->channel_ << " BitRate:" << track->_bitrate << endl;
            //ret = openAudioCodec(track->samplerate_, track->channel_, 64000, codec);
        }

        if (ret) {
            _codec = codec;
            //成功
            logInfo << "打开编码器成功:" << codec->name << ", frameSize " << _context->frame_size;
            // we do not send complete frames, check this
            if (track->trackType_ == "audio") {
                var_frame_size = codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE;
                if (var_frame_size) {
                    logInfo << codec->name << " support var frame_size";
                }
            }
            break;
        }

        if (codec_default && codec_default != codec) {
            //硬件编解码器打开失败，尝试软件的
            logWarn << "打开编码器" << codec->name << "失败，原因是:" << ffmpeg_err(ret) << ", 再尝试打开编码器"
                  << codec_default->name;
            codec = codec_default;
            continue;
        }
        throw std::runtime_error(string("打开编码器") + codec->name + string("失败:") + ffmpeg_err(ret));
    }
}

FFmpegEncoder::~FFmpegEncoder() {
    //logInfo << "~FFmpegEncoder" << endl;
    //_ffmpegencodePool.reset();
    //flush();
}

bool FFmpegEncoder::openVideoCodec(int width, int height, int bitrate, const AVCodec *codec) {
    _context.reset(avcodec_alloc_context3(codec), [](AVCodecContext *ctx) { avcodec_free_context(&ctx); });
    if (!bitrate) {bitrate = 2048000;}
    if (_context) {
        // setupContext(_context.get(), bitrate, 25);

        // 不设置时，仅第一个I帧前存一次sps和pps
        // 设置后，I帧钱不会存储sps和pps, 但_context->extradata会有数据，需要手动处理
        // _context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        _context->width = _option.width ? _option.width : width;
        _context->height = _option.height ? _option.height : height;
        _context->time_base = (AVRational){1, 25};
        _context->framerate = (AVRational){25, 1};
        _context->bit_rate = bitrate;
        _context->rc_min_rate = bitrate;
        _context->rc_max_rate = bitrate; 
        _context->bit_rate_tolerance = bitrate;
        _context->rc_buffer_size = bitrate;
        _context->rc_initial_buffer_occupancy = _context->rc_buffer_size * 3 / 4;
        // _context->rc_buffer_aggressivity = (float)1.0;
        // _context->rc_initial_cplx = 0.5;
        // gop
        _context->gop_size = _option.gop ? _option.gop : 50;
        // 禁用b帧
        _context->max_b_frames = 0;
        _context->has_b_frames = 0;
        //logInfo << "openVideoCodec " << codec->name << " " << _context->width << "x" << _context->height;
        _context->pix_fmt = AV_PIX_FMT_YUV420P; // codec->pix_fmts[0];
        // sws_.reset(new FFmpegSwsCustom(_context->pix_fmt, _context->width, _context->height));
        return avcodec_open2(_context.get(), codec, NULL/*&_dict*/) >= 0;
        // return avcodec_open2(_context.get(), codec, NULL) >= 0;
    }
    return false;
}

bool FFmpegEncoder::openAudioCodec(int samplerate, int channel, int bitrate, const AVCodec *codec) {
    _context.reset(avcodec_alloc_context3(codec), [](AVCodecContext *ctx) { avcodec_free_context(&ctx); });

    if (_context) {
        setupContext(_context.get(), bitrate, samplerate);

        _context->sample_fmt = codec->sample_fmts[0];
        _context->sample_rate = samplerate;
        // _context->channels = channel;
        // _context->channel_layout = av_get_default_channel_layout(_context->channels);
        av_channel_layout_default(&_context->ch_layout, channel);
        //logInfo << "_context->channels:+++++++++++"  << _context->channels << "|| _context->channel_layout:" << _context->channel_layout << endl;

        if (getCodecId() == "opus")
            _context->compression_level = 1;

        //_sample_bytes = av_get_bytes_per_sample(_context->sample_fmt) * _context->channels;
        _swr.reset(new FFmpegSwr(_context->sample_fmt, _context->ch_layout, samplerate));//初始化采样率写成8000

        //logInfo << "openAudioCodec " << codec->name << " " << _context->sample_rate << "x" << _context->channels;
        return avcodec_open2(_context.get(), codec, NULL) >= 0;
    }
    return false;
}

void FFmpegEncoder::flush() {
    while (true) {
        auto packet = alloc_av_packet();
        auto ret = avcodec_receive_packet(_context.get(), packet.get());
        if (ret == AVERROR(EAGAIN)) {
            avcodec_send_frame(_context.get(), nullptr);
            continue;
        }
        if (ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            logWarn << "avcodec_receive_frame failed:" << ffmpeg_err(ret);
            break;
        }
        onEncode(packet.get());
    }
}

//解码后重新编码CSL  inputFrame->inputFrame_l->encodeFrame->onEncode
bool FFmpegEncoder::inputFrame(const FFmpegFrame::Ptr &frame, bool async) {
    // if(!_workPool){
    //     //logInfo << "ffmpegencodePool first" << endl;
    //     //_ffmpegencodePool = WorkThreadPool::Instance().getExecutor();
    //     _ffmpegencodePool = WorkThreadPool::Instance().getPoller();
    //     //_ffmpegencodePool = WorkThreadPool::Instance().getPoller_polling();
    //     //logInfo << "ffmpegencodePool:" << _ffmpegencodePool <<  endl;
    // }
        // weak_ptr<FFmpegEncoderCustom> weakSelf = dynamic_pointer_cast<FFmpegEncoderCustom>(shared_from_this());
        // _ffmpegencodePool->async([weakSelf,frame](){
        //     auto strongSelf = weakSelf.lock();
        //     if (!strongSelf) {
        //         return;
        //     }
            //logInfo << "异步编码" << endl;
            if(frame){
                inputFrame_l(frame);//异步操作
            }
            else{
                logInfo << "Encoder frame is NULL" << endl;
            }
	    // }, false);  

    return true;
}

bool FFmpegEncoder::inputFrame_l(FFmpegFrame::Ptr input) {
    //logInfo << "FFmpegEncoder::inputFrame_l" << endl;
    AVFrame *frame = input->get();
    AVCodecContext *context = _context.get();
    if (_track->trackType_ == "audio") {
        //logInfo << "FFmpegEncoderCustom::inputFrame_l__111" << endl;
        // if (_swr) {
        //     //logInfo << "FFmpegEncoderCustom::inputFrame_l__222" << endl;
        //     // 转成同样采样率和通道

        //     input = _swr->inputFrame(input);//重采样去除，性能优化
            
        //     frame = input->get();
        //     // 保证每次塞给解码器的都是一帧音频
        //     if (!var_frame_size && _context->frame_size && frame->nb_samples != _context->frame_size) {
        //         // add this frame to _audio_buffer
        //         //logInfo << "FFmpegEncoderCustom::inputFrame_l__333" << endl;
        //         if (!_fifo)
        //             _fifo.reset(new FFmpegAudioFifoCustom());
        //         // TraceL << "in " << frame->pts << ",samples " << frame->nb_samples;
        //         //logInfo << "in " << frame->pts << ",samples " << frame->nb_samples;
        //         _fifo->Write(frame);
        //         while (1) {
        //             //logInfo << "FFmpegEncoderCustom::inputFrame_l__444" << endl;
        //             FFmpegFrameCustom audio_frame;
        //             //logInfo << "FFmpegEncoderCustom::inputFrame_l__555" << endl;
        //             if (!_fifo->Read(audio_frame.get(), _context->frame_size)){
        //                 //logInfo << "FFmpegEncoderCustom::inputFrame_l__666" << endl;
        //                 break;
        //             }
        //             if (!encodeFrame(audio_frame.get())) {
        //                 //logInfo << "FFmpegEncoderCustom::inputFrame_l__777" << endl;
        //                 break;
        //             }
        //         }
        //         //logInfo << "FFmpegEncoderCustom::inputFrame_l__888" << endl;
        //         return true;
        //     }
        // }
    } else {
        //logInfo << "FFmpegEncoderCustom::inputFrame_l__999" << endl;
        if (frame->format != context->pix_fmt || frame->width != context->width || frame->height != context->height) {
            if (!_sws) {
                _sws = make_shared<FFmpegSws>(context->pix_fmt, context->width, context->height);
            }
            if (_sws) {
                input = _sws->inputFrame(input);
                frame = input->get();
            } else {
                // @todo reopen videocodec?
                logInfo << "reopen video codec 1024000";
                openVideoCodec(frame->width, frame->height, 1024000, _codec);
            }
        }
    }
    return encodeFrame(frame);
}

bool FFmpegEncoder::encodeFrame(AVFrame *frame) {
    // TraceL << "enc " << frame->pts;
    //logInfo << "avcodec_send_frame+++++" << endl;
    //avcodec_send_frame 负责给编码器提供未压缩的原始数据
    if (_codecId == "h264" || _codecId == "h265") {
        frame->pts = _pts;
        _pts += 1;
    }
    int ret = avcodec_send_frame(_context.get(), frame);//重采样之后的数据发送到编码线程 CSL 
    //logInfo << "avcodec_send_frame+++++ " << ret << endl;
    if (ret < 0) {
        logWarn << "Error sending a frame " << frame->pts << " to the encoder: " << ffmpeg_err(ret);
        return false;
    }
    while (ret >= 0) {
        auto packet = alloc_av_packet();
        ret = avcodec_receive_packet(_context.get(), packet.get());//从编码器取出编码后的数据
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            logWarn << "Error encoding a frame: " << ffmpeg_err(ret);
            return false;
        }
        // TraceL << "out " << packet->pts << "," << packet->dts << ", size: " << packet->size;
        logInfo << "out " << packet->pts << "," << packet->dts << ", size: " << packet->size;
        onEncode(packet.get());
    }
    return true;
}

// extern int dumpAacConfig(const string &config, size_t length, uint8_t *out, size_t out_size);

void FFmpegEncoder::onEncode(AVPacket *packet) {
    // process frame
    //logInfo << "enter encode process frame" << endl;
    //logInfo << "FFmpegEncoder::onEncode  _codecId:" << _codecId << endl;
    if (!_cb)
        return;
    if (_codecId == "h264") {
            int startSize = FrameBuffer::getVideoStartSize((const uint8_t*)packet->data, packet->size);
            auto frame = FrameBuffer::createFrame(_codecId, startSize, VideoTrackType, !startSize);
            //H264Frame::Ptr frame = std::make_shared<H264Frame>();
            frame->_dts = packet->dts * 40;
            frame->_pts = packet->pts * 40;
            // frame->_dts = packet->dts;
            // frame->_pts = packet->pts;
            frame->_buffer.assign((const char *)packet->data, packet->size);
            _cb(frame);
    } else if (_codecId == "h265") {
            int startSize = FrameBuffer::getVideoStartSize((const uint8_t*)packet->data, packet->size);
            auto frame = FrameBuffer::createFrame(_codecId, startSize, VideoTrackType, !startSize);
            //H265Frame::Ptr frame = std::make_shared<H265Frame>();
            frame->_dts = packet->dts;
            frame->_pts = packet->pts;
            frame->_buffer.assign((const char *)packet->data, packet->size);
            _cb(frame);
    } else if (_codecId == "aac") {
            // auto frame = FrameImp::create<>();
            // frame->_codec_id = _codecId;
            // frame->_dts = packet->dts;
            // frame->_pts = packet->pts;
            // frame->_buffer.reserve(ADTS_HEADER_LEN + packet->size);
            // if (_context && _context->extradata && _context->extradata_size) {
            //     uint8_t adts[ADTS_HEADER_LEN];
            //     auto cfg = std::string((const char *)_context->extradata, _context->extradata_size);
            //     dumpAacConfig(cfg, packet->size, adts, ADTS_HEADER_LEN);
            //     frame->_prefix_size = ADTS_HEADER_LEN;
            //     frame->_buffer.append((char*)adts, ADTS_HEADER_LEN);
            // }
            // frame->_buffer.append((const char *)packet->data, packet->size);
            // _cb(frame);
    } else if (_codecId == "g711a" ||_codecId == "g711u" || _codecId == "opus") {
            //logInfo << "FFmpegEncoderCustom::onEncode G711" << endl;
            FrameBuffer::Ptr frame = FrameBuffer::createFrame(_codecId, 0, AudioTrackType, 0);
            // frame->_codec_id = _codecId;
            frame->_dts = packet->dts;
            frame->_pts = packet->pts;
            frame->_buffer.assign((const char *)packet->data, packet->size);
            _cb(frame);
    } else if (_codecId == "vp8" || _codecId == "vp9") {
            auto frame = FrameBuffer::createFrame(_codecId, 0, VideoTrackType, 0);
            // frame->_codec_id = _codecId;
            frame->_dts = packet->dts;
            frame->_pts = packet->pts;
            frame->_buffer.assign((const char *)packet->data, packet->size);
            _cb(frame);
    } else {
        logWarn << "FFmpegEncoder::onEncode unknown codec id: " << _codecId;
    }
}

FfmpegFormat::FfmpegFormat(const std::string& filename) {
    _filename = filename;
}

FfmpegFormat::~FfmpegFormat() {
    if (_videoCodecContext) {
        avcodec_free_context(&_videoCodecContext);
    }
    if (_audioCodecContext) {
        avcodec_free_context(&_audioCodecContext);
    }
    if (_formatContext) {
        avformat_close_input(&_formatContext);
    }
    if (_packet) {
        av_packet_free(&_packet);
    }
    if (_frame) {
        av_frame_free(&_frame);
    }
}

bool FfmpegFormat::open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret, stream_index;
    AVStream *st;
    const AVCodec *dec = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
                av_get_media_type_string(type), _filename);
        return ret;
    } else {
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];

        /* find decoder for the stream */
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        /* Allocate a codec context for the decoder */
        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx) {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                    av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }

        /* Copy codec parameters from input stream to output codec context */
        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                    av_get_media_type_string(type));
            return ret;
        }

        /* Init the decoders */
        if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
        *stream_idx = stream_index;
    }

    return 0;
}

bool FfmpegFormat::open() {
    /* open input file, and allocate format context */
    if (avformat_open_input(&_formatContext, _filename.c_str(), NULL, NULL) < 0) {
        logError << "Could not open source file " << _filename;
        return false;
    }
    /* retrieve stream information */
    if (avformat_find_stream_info(_formatContext, NULL) < 0) {
        logError << "Could not find stream information " << _filename;
        return false;
    }

    if (open_codec_context(&_videoStreamIndex, &_videoCodecContext, _formatContext, AVMEDIA_TYPE_VIDEO) >= 0) {
        _videoStream = _formatContext->streams[_videoStreamIndex];
    }

    if (open_codec_context(&_audioStreamIndex, &_audioCodecContext, _formatContext, AVMEDIA_TYPE_AUDIO) >= 0) {
        _audioStream = _formatContext->streams[_audioStreamIndex];
    }

    /* dump input information to stderr */
    av_dump_format(_formatContext, 0, _filename.c_str(), 0);

    if (!_audioStream && !_videoStream) {
        logError << "Could not find audio or video stream in the input, aborting";
        return false;
    }

    _frame = av_frame_alloc();
    if (!_frame) {
        logError << "Could not allocate frame";
        return false;
    }

    _packet = av_packet_alloc();
    if (!_packet) {
        logError << "Could not allocate packet";
        return false;
    }

    if (_videoStream)
        logInfo << "Demuxing video from file " << _filename;
    if (_audioStream)
        logInfo << "Demuxing audio from file " << _filename;

    return true;
}

static std::string getCodecName(AVCodecID codec_id) {
    switch (codec_id) {
    case AV_CODEC_ID_H264:
        return "h264";
    case AV_CODEC_ID_H265:
        return "h265";
    case AV_CODEC_ID_VP8:
        return "vp8";
    case AV_CODEC_ID_VP9:
        return "vp9";
    case AV_CODEC_ID_AAC:
        return "aac";
    case AV_CODEC_ID_PCM_S16LE:
        return "pcm_s16le";
    case AV_CODEC_ID_PCM_ALAW:
        return "g711a";
    case AV_CODEC_ID_PCM_MULAW:
        return "g711u";
    default:
        return "";
    }
}

FrameBuffer::Ptr FfmpegFormat::readPacket() {
    FrameBuffer::Ptr frame;
    int ret = av_read_frame(_formatContext, _packet);
    if (ret < 0) {
        logError << "Could not read frame: " << ret;
        return frame;
    }

    if (_packet->stream_index == _videoStreamIndex) {
        frame = FrameBuffer::createFrame(getCodecName(_videoCodecContext->codec_id), 0, VideoTrackType, 0);
    } else if (_packet->stream_index == _audioStreamIndex) {
        frame = FrameBuffer::createFrame(getCodecName(_audioCodecContext->codec_id), 0, AudioTrackType, 0);
    }

    if (frame) {
        frame->_dts = _packet->dts;
        frame->_pts = _packet->pts;
        frame->_buffer.assign((const char *)_packet->data, _packet->size);
    }

    return frame;
}

#endif//ENABLE_FFMPEG
