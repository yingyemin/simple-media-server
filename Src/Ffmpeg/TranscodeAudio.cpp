/*
 * Copyright (c) 2016 The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#if defined(ENABLE_FFMPEG)

#include <float.h>

#include "TranscodeAudio.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"
#include "Codec/AacTrack.h"
#include "Codec/G711Track.h"
#include "Codec/H264Frame.h"
#include "Codec/H265Frame.h"
#include "Log/Logger.h"

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

FFmpegFrame::FFmpegFrame(std::shared_ptr<AVFrame> frame) {
    //logInfo << "FFmpegFrame构造" << endl;
    if (frame) {
        _frame = std::move(frame);
    } else {
        _frame.reset(av_frame_alloc(), [](AVFrame *ptr) {
            av_frame_free(&ptr);
        });
    }
}

FFmpegFrame::~FFmpegFrame() {
    //logInfo << "~FFmpegFrame" << endl;
    if (_data) {
        delete[] _data;
        _data = nullptr;
    }
}

AVFrame *FFmpegFrame::get() const {
    return _frame.get();
}

void FFmpegFrame::fillPicture(AVPixelFormat target_format, int target_width, int target_height) {
    assert(_data == nullptr);
    _data = new char[av_image_get_buffer_size(target_format, target_width, target_height, 32)];
    av_image_fill_arrays(_frame->data, _frame->linesize, (uint8_t *) _data,  target_format, target_width, target_height, 32);
}

//AudioDecoder::AudioDecoder(const Track::Ptr &track, int thread_num) {
AudioDecoder::AudioDecoder(const TrackInfo::Ptr &track) {
    const AVCodec *codec = nullptr;
    const AVCodec *codec_default = nullptr;
    if (track->codec_ == "aac") {
        codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    } else if (track->codec_ == "g711a") {
        codec = avcodec_find_decoder(AV_CODEC_ID_PCM_ALAW);
    } else if (track->codec_ == "g711u") {
        codec = avcodec_find_decoder(AV_CODEC_ID_PCM_MULAW);
    } else if (track->codec_ == "opus") {
        codec = avcodec_find_decoder(AV_CODEC_ID_OPUS);
    }

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

        //保存AVFrame的引用
#ifdef FF_API_OLD_ENCDEC
        _context->refcounted_frames = 1;
#endif
        _context->flags |= AV_CODEC_FLAG_LOW_DELAY;
        _context->flags2 |= AV_CODEC_FLAG2_FAST;
        if (track->trackType_ == "audio") {
            if (track->codec_ == "g711a" || track->codec_ == "g711u") {
                _context->sample_rate = track->samplerate_;
                av_channel_layout_default(&_context->ch_layout, track->channel_);
            }
        }

        int ret = avcodec_open2(_context.get(), codec, NULL);
        if (ret >= 0) {
            //成功
            logInfo << "打开解码器成功:" << codec->name;
            break;
        }

        if (codec_default && codec_default != codec) {
            //硬件编解码器打开失败，尝试软件的
            logWarn << "打开解码器" << codec->name << "失败，原因是:" << ffmpeg_err(ret) << ", 再尝试打开解码器" << codec_default->name;
            codec = codec_default;
            continue;
        }
        throw std::runtime_error("打开解码器" + string(codec->name) + "失败:" + ffmpeg_err(ret));
    }
}

AudioDecoder::~AudioDecoder() {
    //flush();
}

void AudioDecoder::flush() {
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

//此处根据音频包类型进行转码转发
bool AudioDecoder::inputFrame(const FrameBuffer::Ptr &frame, bool live, bool async, bool enable_merge) {
    auto pkt = alloc_av_packet();
    pkt->data = (uint8_t *) frame->data();
    pkt->size = frame->size();
    pkt->dts = frame->dts();
    pkt->pts = frame->pts();
    //logInfo << "decodeFrame size:"<< size << " dts:" << dts << " pts:"<< pts << "" <<  " live:" << live << endl;
    auto ret = avcodec_send_packet(_context.get(), pkt.get());//_context创建解码器的相关内容
    if (ret < 0) {
        //logInfo << "enter decodeFrame_22222" << endl;
        if (ret != AVERROR_INVALIDDATA) {
            logWarn << "avcodec_send_packet failed:" << ffmpeg_err(ret);
        }
        return false;
    }
    //第一次调用avcodec_send_packet，avcodec_receive_frame就成功接收到数据了。并且以后每次也都一一对应调用两个函数
    while (true) {
        //logInfo << "AudioDecoder decodeFrame" << endl;
        auto out_frame = std::make_shared<FFmpegFrame>();
        
        //avcodec_receive_frame 从解码器内部缓存中提取解码后的音视频帧 ret=0：表示可以提取到解码后的音视频帧
        ret = avcodec_receive_frame(_context.get(), out_frame->get());
        //logInfo << "avcodec_receive_frame ======= ret" << "|| " << ret << endl;
        //ret == AVERROR(EAGAIN): 表示当前解码没有任何问题，但是输入的packet数据不够
        //ret == AVERROR_EOF：内部缓冲区中数据全部解码完成，不再有解码后的帧数据输出
        //ret < 0 送入输入数据包失败
        //ret = 0 解码正常
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            logWarn << "avcodec_receive_frame failed:" << ffmpeg_err(ret);
            break;
        }
        // if (live && pts - out_frame->get()->pts > MAX_DELAY_SECOND * 1000 && _ticker.createdTime() > 10 * 1000) {
        //     //后面的帧才忽略,防止Track无法ready
        //     logWarn << "解码时，忽略" << MAX_DELAY_SECOND << "秒前的数据:" << pts << " " << out_frame->get()->pts;
        //     continue;
        // }
        onDecode(out_frame);
    }
    return true;
}

void AudioDecoder::setOnDecode(const std::function<void(const FFmpegFrame::Ptr &)>& cb) {
    _onDecode = std::move(cb);
}

void AudioDecoder::onDecode(const FFmpegFrame::Ptr &frame) {
    if (_onDecode) {
        _onDecode(frame);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

AudioFifo::~AudioFifo() {
    if (_fifo) {
        av_audio_fifo_free(_fifo);
        _fifo = nullptr;
    }
}

int AudioFifo::size() const {
    return _fifo ? av_audio_fifo_size(_fifo) : 0;
}

bool AudioFifo::write(const AVFrame *frame) {
    //logInfo << "AudioFifo::Write" << endl;
    _format = (AVSampleFormat)frame->format;
    if (!_fifo) {
        //logInfo << "AudioFifo::Write " << "frame->channels: " << frame->channels << " frame->nb_samples:" << frame->nb_samples << endl;
        _fifo = av_audio_fifo_alloc(_format, frame->ch_layout.nb_channels, frame->nb_samples);
        if (!_fifo) {
            logWarn << "av_audio_fifo_alloc " << frame->ch_layout.nb_channels << "x" << frame->nb_samples << "error";
            return false;
        }
    }

    _channels = frame->ch_layout.nb_channels;
    if (_samplerate != frame->sample_rate) {
        _samplerate = frame->sample_rate;
        // 假定传入frame的时间戳是以ms为单位的
        _timebase = 1000.0 / _samplerate;
    }
    
    logTrace << "_channels: " << _channels << ", _samplerate: " << _samplerate << ", pts : " << frame->pts;
    if (frame->pts != AV_NOPTS_VALUE) {
        // 计算fifo audio第一个采样的时间戳
        // double tsp = frame->pts - _timebase * av_audio_fifo_size(_fifo);
        // // flv.js和webrtc对音频时间戳增量有要求, rtc要求更加严格！
        // // 得尽量保证时间戳是按照sample_size累加，否则容易出现破音或杂音等问题
        // if (fabs(_tsp) < DBL_EPSILON || fabs(tsp - _tsp) > 200) {
        //     logInfo << "reset base_tsp " << _tsp << "->" << tsp;
        //     _tsp = tsp;
        // }
    } else {
        _tsp = 0;
    }

    av_audio_fifo_write(_fifo, (void **)frame->data, frame->nb_samples);
    return true;
}

bool AudioFifo::read(AVFrame *frame, int sample_size) {
    //logInfo << "AudioFifo::Read" << endl;
    assert(_fifo);
    int fifo_size = av_audio_fifo_size(_fifo);
    if (fifo_size < sample_size)
        return false;
    // fill linedata
    //_channels:单个通道中的样本数  _format:样本格式  sample_size:帧的大小
    //logInfo << "AudioFifo::Read " << " frame->linesize:" << frame->linesize << " _channels:" << _channels << " sample_size:" << sample_size << endl;
    av_samples_get_buffer_size(frame->linesize, _channels, sample_size, _format, 0);//获取给定音频参数所需的缓冲区大小
    frame->nb_samples = sample_size;
    frame->format = _format;
    // frame->channel_layout = av_get_default_channel_layout(_channels);//根据声道数目获取声道布局
    av_channel_layout_default(&frame->ch_layout, _channels);
    frame->sample_rate = _samplerate;
    if (fabs(_tsp) > DBL_EPSILON) {
        frame->pts = _tsp;
        // advance tsp by sample_size
        _tsp += sample_size * _timebase;
    }
    else {
        frame->pts = AV_NOPTS_VALUE;
    }

    int ret = av_frame_get_buffer(frame, 0);
    //logInfo << "AudioFifo::Read ret:" << ret << endl;
    if (ret < 0) {
        logWarn << "av_frame_get_buffer error " << ffmpeg_err(ret);
        return false;
    }

    av_audio_fifo_read(_fifo, (void **)frame->data, sample_size);
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

AudioSwr::AudioSwr(AVSampleFormat output, AVChannelLayout channel_layout, int samplerate) {
    _target_format = output;
    // _target_channels = channel;
    // _target_channel_layout = channel_layout;
    av_channel_layout_copy(&_target_channel_layout, &channel_layout);
    _target_samplerate = samplerate;
}

AudioSwr::~AudioSwr() {
    if (_ctx) {
        swr_free(&_ctx);
    }
}

FFmpegFrame::Ptr AudioSwr::inputFrame(const FFmpegFrame::Ptr &frame) {
    if (frame->get()->format == _target_format &&
        // frame->get()->channels == _target_channels &&
        // frame->get()->channel_layout == (uint64_t)_target_channel_layout &&
        !av_channel_layout_compare(&frame->get()->ch_layout, &_target_channel_layout) &&
        frame->get()->sample_rate == _target_samplerate) {
        //不转格式
        return frame;
    }
    if (!_ctx) {
        //配置重采样 参数
        int error = swr_alloc_set_opts2(&_ctx, &_target_channel_layout, _target_format, _target_samplerate,
                                  &frame->get()->ch_layout, (AVSampleFormat) frame->get()->format,
                                  frame->get()->sample_rate, 0, nullptr);
        if (error < 0) {
            throw std::runtime_error("重采样初始化失败");
        }
        logInfo << "sample_rate:" << frame->get()->sample_rate << endl;
        logInfo << "swr_alloc_set_opts2:" << av_get_sample_fmt_name((enum AVSampleFormat) frame->get()->format) << " -> "
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

///////////////////////////////////////////////////////////////////////////////////////////////////////

void setupContext(AVCodecContext *_context, int bitrate) {
    //保存AVFrame的引用
#ifdef FF_API_OLD_ENCDEC
    _context->refcounted_frames = 1;
#endif
    _context->flags |= AV_CODEC_FLAG_LOW_DELAY;
    _context->flags2 |= AV_CODEC_FLAG2_FAST;
    _context->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    _context->time_base.num = 1;
    _context->time_base.den = 1000; // {1, 1000}
    if (bitrate != 0) {
        _context->bit_rate = bitrate;
    }
}

AudioEncoder::AudioEncoder(const TrackInfo::Ptr &track)
    :_trackInfo(track)
{
    //logInfo << "AudioEncoder构造" << endl;
    const AVCodec *codec = nullptr;
    const AVCodec *codec_default = nullptr;
    _codecId = track->codec_;
    
    if (_codecId == "aac") {
        codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    } else if (_codecId == "g711a") {
        codec = avcodec_find_encoder(AV_CODEC_ID_PCM_ALAW);
    } else if (_codecId == "g711u") {
        codec = avcodec_find_encoder(AV_CODEC_ID_PCM_MULAW);
    } else if (_codecId == "opus") {
        codec = avcodec_find_encoder(AV_CODEC_ID_OPUS);
    }

    if (!codec) {
        throw std::runtime_error("未找到编码器");
    }

    while (true) {
        bool ret = false;
        if (track->trackType_ == "audio") {
            // AudioTrack::Ptr audio = static_pointer_cast<AudioTrack>(track);
            ret = initCodec(track->samplerate_, track->channel_, track->_bitrate, codec);
            // logInfo << "音频编码器" << "AudioSampleRate:" << audio->getAudioSampleRate() << " AudioChannel:" << audio->getAudioChannel() << " BitRate:" << track->getBitRate() << endl;
            //ret = openAudioCodec(audio->getAudioSampleRate(), audio->getAudioChannel(), 64000, codec);
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
        throw std::runtime_error("打开编码器" + string(codec->name) + "失败:" + ffmpeg_err(ret));
    }
}

AudioEncoder::~AudioEncoder() {
    //logInfo << "~AudioEncoder" << endl;
    //_ffmpegencodePool.reset();
    //flush();
}

bool AudioEncoder::initCodec(int samplerate, int channel, int bitrate, const AVCodec *codec) {
    _context.reset(avcodec_alloc_context3(codec), [](AVCodecContext *ctx) { avcodec_free_context(&ctx); });

    if (_context) {
        setupContext(_context.get(), bitrate);

        _context->sample_fmt = codec->sample_fmts[0];
        _context->sample_rate = samplerate;
        // _context->channels = channel;
        // _context->channel_layout = av_get_default_channel_layout(_context->channels);
        av_channel_layout_default(&_context->ch_layout, channel);
        //logInfo << "_context->channels:+++++++++++"  << _context->channels << "|| _context->channel_layout:" << _context->channel_layout << endl;

        if (_trackInfo->codec_ == "opus")
            _context->compression_level = 1;

        //_sample_bytes = av_get_bytes_per_sample(_context->sample_fmt) * _context->channels;
        _swr.reset(new AudioSwr(_context->sample_fmt, _context->ch_layout, _trackInfo->samplerate_));//初始化采样率写成8000

        //logInfo << "openAudioCodec " << codec->name << " " << _context->sample_rate << "x" << _context->channels;
        return avcodec_open2(_context.get(), codec, NULL) >= 0;
    }
    return false;
}

void AudioEncoder::flush() {
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
        onPacket(packet.get());
    }
}

//解码后重新编码CSL  inputFrame->inputFrame_l->encode->onPacket
bool AudioEncoder::inputFrame(const FFmpegFrame::Ptr &input, bool async) {
    AVFrame *frame = input->get();
    AVCodecContext *context = _context.get();
    if (_trackInfo->trackType_ == "audio") {
        //logInfo << "AudioEncoder::inputFrame_l__111" << endl;
        if (_swr) {
            //logInfo << "AudioEncoder::inputFrame_l__222" << endl;
            // 转成同样采样率和通道

            auto inputSwr = _swr->inputFrame(input);//重采样去除，性能优化
            
            frame = inputSwr->get();
            // 保证每次塞给解码器的都是一帧音频
            if (!var_frame_size && _context->frame_size && frame->nb_samples != _context->frame_size) {
                // add this frame to _audio_buffer
                //logInfo << "AudioEncoder::inputFrame_l__333" << endl;
                if (!_fifo)
                    _fifo.reset(new AudioFifo());
                // logTrace << "in " << frame->pts << ",samples " << frame->nb_samples;
                //logInfo << "in " << frame->pts << ",samples " << frame->nb_samples;
                _fifo->write(frame);
                while (1) {
                    //logInfo << "AudioEncoder::inputFrame_l__444" << endl;
                    FFmpegFrame audio_frame;
                    //logInfo << "AudioEncoder::inputFrame_l__555" << endl;
                    if (!_fifo->read(audio_frame.get(), _context->frame_size)){
                        //logInfo << "AudioEncoder::inputFrame_l__666" << endl;
                        break;
                    }
                    if (!encode(audio_frame.get())) {
                        //logInfo << "AudioEncoder::inputFrame_l__777" << endl;
                        break;
                    }
                }
                //logInfo << "AudioEncoder::inputFrame_l__888" << endl;
                return true;
            } else {
                return encode(frame);
            }
        }
    }
    return encode(frame);
}

bool AudioEncoder::encode(AVFrame *frame) {
    // logTrace << "enc " << frame->pts;
    //logInfo << "avcodec_send_frame+++++" << endl;
    //avcodec_send_frame 负责给编码器提供未压缩的原始数据
    frame->pts = _sampleNum * 1000 / _context->sample_rate;
    int ret = avcodec_send_frame(_context.get(), frame);//重采样之后的数据发送到编码线程 CSL 
    _sampleNum += frame->nb_samples;
    logTrace << "_sampleNum: " << _sampleNum << ", samplerate: " << _context->sample_rate 
            << ", frame pts: " << frame->pts;
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
        logTrace << "out " << packet->pts << "," << packet->dts << ", size: " << packet->size;
        onPacket(packet.get());
    }
    return true;
}

void AudioEncoder::onPacket(AVPacket *packet) {
    // process frame
    //logInfo << "enter encode process frame" << endl;
    //logInfo << "AudioEncoder::onPacket  _codecId:" << _codecId << endl;
    if (!_onPakcet)
        return;
    if (_codecId == "aac") {
        auto frame = std::make_shared<FrameBuffer>();
        frame->_codec = _codecId;
        frame->_dts = packet->dts;
        frame->_pts = packet->pts;
        frame->_buffer.reserve(7 + packet->size);
        if (_context && _context->extradata && _context->extradata_size) {
            auto cfg = std::string((const char *)_context->extradata, _context->extradata_size);
            auto track = dynamic_pointer_cast<AacTrack>(_trackInfo);
            track->setAacInfo(cfg);
            auto adts = track->getAdtsHeader(packet->size);
            frame->_startSize = 7;
            frame->_buffer.append(adts.c_str(), 7);
        }
        frame->_buffer.append((const char *)packet->data, packet->size);
        _onPakcet(frame);
    } else if (_codecId == "opus" || _codecId == "g711a" || _codecId == "g711u") {
        //logInfo << "AudioEncoder::onPacket G711" << endl;
        auto frame = std::make_shared<FrameBuffer>();
        frame->_codec = _codecId;
        frame->_dts = packet->dts;
        frame->_pts = packet->pts;
        frame->_buffer.assign((const char *)packet->data, packet->size);
        _onPakcet(frame);
    }
}

#endif//ENABLE_FFMPEG
