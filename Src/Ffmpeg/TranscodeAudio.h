#ifndef TransCodeAudio_H
#define TransCodeAudio_H

#include <string>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>

#include "Common/Frame.h"

class AudioEncodeOption
{
public:
    std::string codec_;
    AVCodecID codeId_;
    int sampleRate_;
    int sampleBit_;
    int channel_;
};

class TransCodeAudio
{
public:
    TransCodeAudio(const AudioEncodeOption& option, AVCodecID deAudioCodecId);
    ~TransCodeAudio();

public:
    void initDecode();
    void initEncode();
    int initResampler(AVCodecContext *input_codec_context,
                          AVCodecContext *output_codec_context,
                          SwrContext **resample_context);

    int initFifo(AVAudioFifo **fifo, AVCodecContext *output_codec_context);

    int decode_audio_frame(AVPacket *input_packet, AVFrame *frame,
                              AVCodecContext *input_codec_context,
                              int *data_present, int *finished);

    int add_samples_to_fifo(AVAudioFifo *fifo,
                               uint8_t **converted_input_samples,
                               const int frame_size);

    int read_decode_convert_and_store(AVPacket *input_packet, AVAudioFifo *fifo,
                                         AVCodecContext *input_codec_context,
                                         AVCodecContext *output_codec_context,
                                         SwrContext *resampler_context,
                                         int *finished);

    int init_output_frame(AVFrame **frame,
                             AVCodecContext *output_codec_context,
                             int frame_size);

    int encode_audio_frame(AVFrame *frame,
                              AVCodecContext *output_codec_context,
                              int *data_present);

    int load_encode_and_write(AVAudioFifo *fifo,
                                 AVCodecContext *output_codec_context);

    int inputFrame(const FrameBuffer::Ptr& frame);

private:
    int64_t _pts = 0;

    AudioEncodeOption _option;
    const AVCodec *_deCodec;
    AVCodecContext *_deCodecCtx= NULL;
    AVCodecParserContext *_deParser = NULL;
    AVPacket *_dePkt;
    AVCodecID _deAudioCodecId;

    AVCodecContext *_enCodecCtx= NULL;
    AVPacket *_enPkt;
    int _bInitEncode = 0;

    SwrContext *_resampleContext;
    AVAudioFifo *_fifo;
};

#endif