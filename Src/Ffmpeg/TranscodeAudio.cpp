extern "C" {
#include <libavutil/mem.h>
#include <libavformat/avio.h>

#include <libavutil/avassert.h>
#include <libavutil/avstring.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
}

#include "TranscodeAudio.h"
#include "Log/Logger.h"

/* The output bit rate in bit/s */
#define OUTPUT_BIT_RATE 96000
/* The number of output channels */
#define OUTPUT_CHANNELS 2

static string getAvError(int errnum)
{
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_err2str_cpp(errnum, buffer, AV_ERROR_MAX_STRING_SIZE);

    return string(buffer);
}

/* check that a given sample format is supported by the encoder */
static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}

/* just pick the highest supported samplerate */
static int select_sample_rate(const AVCodec *codec)
{
    const int *p;
    int best_samplerate = 0;

    if (!codec->supported_samplerates)
        return 44100;

    p = codec->supported_samplerates;
    while (*p) {
        if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
            best_samplerate = *p;
        p++;
    }
    return best_samplerate;
}

/* select layout with the highest channel count */
static int select_channel_layout(const AVCodec *codec, AVChannelLayout *dst)
{
    const AVChannelLayout *p, *best_ch_layout;
    int best_nb_channels   = 0;

    AVChannelLayout layout;
    layout.nb_channels = 2;
    layout.u.mask = AV_CH_LAYOUT_STEREO;
    layout.opaque = NULL;
    layout.order = AV_CHANNEL_ORDER_NATIVE;

    if (!codec->ch_layouts)
        return av_channel_layout_copy(dst, &layout);

    p = codec->ch_layouts;
    while (p->nb_channels) {
        int nb_channels = p->nb_channels;

        if (nb_channels > best_nb_channels) {
            best_ch_layout   = p;
            best_nb_channels = nb_channels;
        }
        p++;
    }
    return av_channel_layout_copy(dst, best_ch_layout);
}

/**
 * Initialize one data packet for reading or writing.
 * @param[out] packet Packet to be initialized
 * @return Error code (0 if successful)
 */
static int init_packet(AVPacket **packet)
{
    if (!(*packet = av_packet_alloc())) {
        fprintf(stderr, "Could not allocate packet\n");
        return AVERROR(ENOMEM);
    }
    return 0;
}

/**
 * Initialize one audio frame for reading from the input file.
 * @param[out] frame Frame to be initialized
 * @return Error code (0 if successful)
 */
static int init_input_frame(AVFrame **frame)
{
    if (!(*frame = av_frame_alloc())) {
        fprintf(stderr, "Could not allocate input frame\n");
        return AVERROR(ENOMEM);
    }
    return 0;
}

/**
 * Initialize a temporary storage for the specified number of audio samples.
 * The conversion requires temporary storage due to the different format.
 * The number of audio samples to be allocated is specified in frame_size.
 * @param[out] converted_input_samples Array of converted samples. The
 *                                     dimensions are reference, channel
 *                                     (for multi-channel audio), sample.
 * @param      output_codec_context    Codec context of the output file
 * @param      frame_size              Number of samples to be converted in
 *                                     each round
 * @return Error code (0 if successful)
 */
static int init_converted_samples(uint8_t ***converted_input_samples,
                                  AVCodecContext *output_codec_context,
                                  int frame_size)
{
    int error;

    /* Allocate as many pointers as there are audio channels.
     * Each pointer will point to the audio samples of the corresponding
     * channels (although it may be NULL for interleaved formats).
     * Allocate memory for the samples of all channels in one consecutive
     * block for convenience. */
    if ((error = av_samples_alloc_array_and_samples(converted_input_samples, NULL,
                                  output_codec_context->ch_layout.nb_channels,
                                  frame_size,
                                  output_codec_context->sample_fmt, 0)) < 0) {
        fprintf(stderr,
                "Could not allocate converted input samples (error '%s')\n",
                getAvError(error).c_str());
        return error;
    }
    return 0;
}

/**
 * Convert the input audio samples into the output sample format.
 * The conversion happens on a per-frame basis, the size of which is
 * specified by frame_size.
 * @param      input_data       Samples to be decoded. The dimensions are
 *                              channel (for multi-channel audio), sample.
 * @param[out] converted_data   Converted samples. The dimensions are channel
 *                              (for multi-channel audio), sample.
 * @param      frame_size       Number of samples to be converted
 * @param      resample_context Resample context for the conversion
 * @return Error code (0 if successful)
 */
static int convert_samples(const uint8_t **input_data,
                           uint8_t **converted_data, const int frame_size,
                           SwrContext *resample_context)
{
    int error;

    /* Convert the samples using the resampler. */
    if ((error = swr_convert(resample_context,
                             converted_data, frame_size,
                             input_data    , frame_size)) < 0) {
        fprintf(stderr, "Could not convert input samples (error '%s')\n",
                getAvError(error).c_str());
        return error;
    }

    return 0;
}

TranscodeAudio::TranscodeAudio(const AudioEncodeOption& option, AVCodecID deAudioCodecId)
    :_option(option)
    ,_deAudioCodecId(deAudioCodecId)
{}

TranscodeAudio::~TranscodeAudio()
{
    int data_written;
    /* Flush the encoder as it may have delayed frames. */
    do {
        if (encode_audio_frame(NULL, _enCodecCtx, &data_written))
            break;
    } while (data_written);

    
    if (_fifo)
        av_audio_fifo_free(_fifo);
    swr_free(&_resampleContext);
    if (_enCodecCtx)
        avcodec_free_context(&_enCodecCtx);
    if (_deCodecCtx)
        avcodec_free_context(&_deCodecCtx);
}

void TranscodeAudio::initDecode()
{
    _dePkt = av_packet_alloc();

    /* find the MPEG audio decoder */
    _deCodec = avcodec_find_decoder(_deAudioCodecId);
    if (!_deCodec) {
        logError << "Codec not found";
        return ;
    }

    if (_deAudioCodecId != AV_CODEC_ID_PCM_ALAW) {
        _deParser = av_parser_init(_deCodec->id);
        if (!_deParser) {
            logError << "Parser not found";
            return ;
        }
    }

    _deCodecCtx = avcodec_alloc_context3(_deCodec);
    if (!_deCodecCtx) {
        logError << "Could not allocate audio codec context";
        return ;
    }

    if (_deAudioCodecId == AV_CODEC_ID_PCM_ALAW) {
        // _deCodecCtx->sample_fmt = _deCodec->sample_fmts[0];
        _deCodecCtx->sample_rate = 8000;
        av_channel_layout_default(&_deCodecCtx->ch_layout, 1);
    }

    /* open it */
    if (avcodec_open2(_deCodecCtx, _deCodec, NULL) < 0) {
        logError << "Could not open codec";
        return ;
    }
}

void TranscodeAudio::initEncode()
{
    const AVCodec *codec;
    /* find the MP2 encoder */
    codec = avcodec_find_encoder(_option.codeId_);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        return ;
    }

    _enCodecCtx = avcodec_alloc_context3(codec);
    if (!_enCodecCtx) {
        fprintf(stderr, "Could not allocate audio codec context\n");
        return ;
    }

    /* put sample parameters */
    // _enCodecCtx->bit_rate = 64000;

    /* check that the encoder supports s16 pcm input */
    _enCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    _enCodecCtx->profile = FF_PROFILE_AAC_LOW;
    if (!check_sample_fmt(codec, _enCodecCtx->sample_fmt)) {
        fprintf(stderr, "Encoder does not support sample format %s",
                av_get_sample_fmt_name(_enCodecCtx->sample_fmt));
        return ;
    }

    /* select other audio parameters supported by the encoder */
    _enCodecCtx->sample_rate    = _option.sampleRate_; //select_sample_rate(codec);
    // int ret = select_channel_layout(codec, &_enCodecCtx->ch_layout);
    av_channel_layout_default(&_enCodecCtx->ch_layout, 2);
    // if (ret < 0)
    //     return ;

    /* open it */
    if (avcodec_open2(_enCodecCtx, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        return ;
    }
}

void TranscodeAudio::init()
{
    initDecode();
    initEncode();
    initFifo(&_fifo, _enCodecCtx);
    initResampler(_deCodecCtx, _enCodecCtx, &_resampleContext);
}

/**
 * Initialize the audio resampler based on the input and output codec settings.
 * If the input and output sample formats differ, a conversion is required
 * libswresample takes care of this, but requires initialization.
 * @param      input_codec_context  Codec context of the input file
 * @param      output_codec_context Codec context of the output file
 * @param[out] resample_context     Resample context for the required conversion
 * @return Error code (0 if successful)
 */
int TranscodeAudio::initResampler(AVCodecContext *input_codec_context,
                          AVCodecContext *output_codec_context,
                          SwrContext **resample_context)
{
        int error;

        /*
         * Create a resampler context for the conversion.
         * Set the conversion parameters.
         */
        error = swr_alloc_set_opts2(resample_context,
                                             &output_codec_context->ch_layout,
                                              output_codec_context->sample_fmt,
                                              output_codec_context->sample_rate,
                                             &input_codec_context->ch_layout,
                                              input_codec_context->sample_fmt,
                                              input_codec_context->sample_rate,
                                              0, NULL);
        if (error < 0) {
            fprintf(stderr, "Could not allocate resample context\n");
            return error;
        }
        /*
        * Perform a sanity check so that the number of converted samples is
        * not greater than the number of samples to be converted.
        * If the sample rates differ, this case has to be handled differently
        */
        // av_assert0(output_codec_context->sample_rate == input_codec_context->sample_rate);

        /* Open the resampler with the specified parameters. */
        if ((error = swr_init(*resample_context)) < 0) {
            fprintf(stderr, "Could not open resample context\n");
            swr_free(resample_context);
            return error;
        }
    return 0;
}

/**
 * Initialize a FIFO buffer for the audio samples to be encoded.
 * @param[out] fifo                 Sample buffer
 * @param      output_codec_context Codec context of the output file
 * @return Error code (0 if successful)
 */
int TranscodeAudio::initFifo(AVAudioFifo **fifo, AVCodecContext *output_codec_context)
{
    /* Create the FIFO buffer based on the specified output sample format. */
    if (!(*fifo = av_audio_fifo_alloc(output_codec_context->sample_fmt,
                                      output_codec_context->ch_layout.nb_channels, 1))) {
        fprintf(stderr, "Could not allocate FIFO\n");
        return AVERROR(ENOMEM);
    }
    return 0;
}

/**
 * Decode one audio frame from the input file.
 * @param      frame                Audio frame to be decoded
 * @param      input_format_context Format context of the input file
 * @param      input_codec_context  Codec context of the input file
 * @param[out] data_present         Indicates whether data has been decoded
 * @param[out] finished             Indicates whether the end of file has
 *                                  been reached and all data has been
 *                                  decoded. If this flag is false, there
 *                                  is more data to be decoded, i.e., this
 *                                  function has to be called again.
 * @return Error code (0 if successful)
 */
int TranscodeAudio::decode_audio_frame(AVPacket *input_packet, AVFrame *frame,
                              AVCodecContext *input_codec_context,
                              int *data_present, int *finished)
{
    /* Packet used for temporary storage. */
    // AVPacket *input_packet;
    int error;

    // error = init_packet(&input_packet);
    // if (error < 0)
    //     return error;

    *data_present = 0;
    *finished = 0;
    /* Read one audio frame from the input file into a temporary packet. */
    // if ((error = av_read_frame(input_format_context, input_packet)) < 0) {
        /* If we are at the end of the file, flush the decoder below. */
        // if (error == AVERROR_EOF)
        //     *finished = 1;
        // else {
        //     fprintf(stderr, "Could not read frame (error '%s')\n",
        //             av_err2str(error));
        //     goto cleanup;
        // }
    // }
    do {
        /* Send the audio frame stored in the temporary packet to the decoder.
        * The input audio stream decoder is used to do this. */
        if ((error = avcodec_send_packet(input_codec_context, input_packet)) < 0) {
            fprintf(stderr, "Could not send packet for decoding (error '%s')\n",
                    getAvError(error).c_str());
            break;
        }

        /* Receive one frame from the decoder. */
        error = avcodec_receive_frame(input_codec_context, frame);
        /* If the decoder asks for more data to be able to decode a frame,
        * return indicating that no data is present. */
        if (error == AVERROR(EAGAIN)) {
            error = 0;
            break;
        /* If the end of the input file is reached, stop decoding. */
        } else if (error == AVERROR_EOF) {
            *finished = 1;
            error = 0;
            break;
        } else if (error < 0) {
            printf("Could not decode frame (error '%s')\n",
                    getAvError(error).c_str());
            break;
        /* Default case: Return decoded data. */
        } else {
            *data_present = 1;
            break;
        }
    } while (0);


    // av_packet_free(&input_packet);
    return error;
}

/**
 * Add converted input audio samples to the FIFO buffer for later processing.
 * @param fifo                    Buffer to add the samples to
 * @param converted_input_samples Samples to be added. The dimensions are channel
 *                                (for multi-channel audio), sample.
 * @param frame_size              Number of samples to be converted
 * @return Error code (0 if successful)
 */
int TranscodeAudio::add_samples_to_fifo(AVAudioFifo *fifo,
                               uint8_t **converted_input_samples,
                               const int frame_size)
{
    int error;

    /* Make the FIFO as large as it needs to be to hold both,
     * the old and the new samples. */
    if ((error = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frame_size)) < 0) {
        fprintf(stderr, "Could not reallocate FIFO\n");
        return error;
    }

    /* Store the new samples in the FIFO buffer. */
    if (av_audio_fifo_write(fifo, (void **)converted_input_samples,
                            frame_size) < frame_size) {
        fprintf(stderr, "Could not write data to FIFO\n");
        return AVERROR_EXIT;
    }
    return 0;
}

/**
 * Read one audio frame from the input file, decode, convert and store
 * it in the FIFO buffer.
 * @param      fifo                 Buffer used for temporary storage
 * @param      input_format_context Format context of the input file
 * @param      input_codec_context  Codec context of the input file
 * @param      output_codec_context Codec context of the output file
 * @param      resampler_context    Resample context for the conversion
 * @param[out] finished             Indicates whether the end of file has
 *                                  been reached and all data has been
 *                                  decoded. If this flag is false,
 *                                  there is more data to be decoded,
 *                                  i.e., this function has to be called
 *                                  again.
 * @return Error code (0 if successful)
 */
int TranscodeAudio::read_decode_convert_and_store(AVPacket *input_packet, AVAudioFifo *fifo,
                                         AVCodecContext *input_codec_context,
                                         AVCodecContext *output_codec_context,
                                         SwrContext *resampler_context,
                                         int *finished)
{
    /* Temporary storage of the input samples of the frame read from the file. */
    AVFrame *input_frame = NULL;
    /* Temporary storage for the converted input samples. */
    uint8_t **converted_input_samples = NULL;
    int data_present;
    int ret = AVERROR_EXIT;

    do {
        /* Initialize temporary storage for one input frame. */
        if (init_input_frame(&input_frame))
            break;
        /* Decode one frame worth of audio samples. */
        if (decode_audio_frame(input_packet, input_frame, input_codec_context, &data_present, finished))
            break;
        /* If we are at the end of the file and there are no more samples
        * in the decoder which are delayed, we are actually finished.
        * This must not be treated as an error. */
        if (*finished) {
            ret = 0;
            break;
        }
        /* If there is decoded data, convert and store it. */
        if (data_present) {
            /* Initialize the temporary storage for the converted input samples. */
            if (init_converted_samples(&converted_input_samples, output_codec_context,
                                    input_frame->nb_samples))
                break;

            /* Convert the input samples to the desired output sample format.
            * This requires a temporary storage provided by converted_input_samples. */
            if (convert_samples((const uint8_t**)input_frame->extended_data, converted_input_samples,
                                input_frame->nb_samples, resampler_context))
                break;

            /* Add the converted input samples to the FIFO buffer for later processing. */
            if (add_samples_to_fifo(fifo, converted_input_samples,
                                    input_frame->nb_samples))
                break;
            ret = 0;
        }
        ret = 0;
    } while (0);

    if (converted_input_samples)
        av_freep(&converted_input_samples[0]);
    av_freep(&converted_input_samples);
    av_frame_free(&input_frame);

    return ret;
}

/**
 * Initialize one input frame for writing to the output file.
 * The frame will be exactly frame_size samples large.
 * @param[out] frame                Frame to be initialized
 * @param      output_codec_context Codec context of the output file
 * @param      frame_size           Size of the frame
 * @return Error code (0 if successful)
 */
int TranscodeAudio::init_output_frame(AVFrame **frame,
                             AVCodecContext *output_codec_context,
                             int frame_size)
{
    int error;

    /* Create a new frame to store the audio samples. */
    if (!(*frame = av_frame_alloc())) {
        fprintf(stderr, "Could not allocate output frame\n");
        return AVERROR_EXIT;
    }

    /* Set the frame's parameters, especially its size and format.
     * av_frame_get_buffer needs this to allocate memory for the
     * audio samples of the frame.
     * Default channel layouts based on the number of channels
     * are assumed for simplicity. */
    (*frame)->nb_samples     = frame_size;
    av_channel_layout_copy(&(*frame)->ch_layout, &output_codec_context->ch_layout);
    (*frame)->format         = output_codec_context->sample_fmt;
    (*frame)->sample_rate    = output_codec_context->sample_rate;

    /* Allocate the samples of the created frame. This call will make
     * sure that the audio frame can hold as many samples as specified. */
    if ((error = av_frame_get_buffer(*frame, 0)) < 0) {
        fprintf(stderr, "Could not allocate output frame samples (error '%s')\n",
                getAvError(error).c_str());
        av_frame_free(frame);
        return error;
    }

    return 0;
}

static void getADTSHeader(AVCodecContext *ctx, uint8_t *adts_header, int aac_length)
{
    uint8_t freq_idx = 0;    //0: 96000 Hz  3: 48000 Hz 4: 44100 Hz
    switch (ctx->sample_rate) {
        case 96000: freq_idx = 0; break;
        case 88200: freq_idx = 1; break;
        case 64000: freq_idx = 2; break;
        case 48000: freq_idx = 3; break;
        case 44100: freq_idx = 4; break;
        case 32000: freq_idx = 5; break;
        case 24000: freq_idx = 6; break;
        case 22050: freq_idx = 7; break;
        case 16000: freq_idx = 8; break;
        case 12000: freq_idx = 9; break;
        case 11025: freq_idx = 10; break;
        case 8000: freq_idx = 11; break;
        case 7350: freq_idx = 12; break;
        default: freq_idx = 4; break;
    }
    uint8_t chanCfg = ctx->ch_layout.nb_channels;
    uint32_t frame_length = aac_length + 7;
    adts_header[0] = 0xFF;
    adts_header[1] = 0xF1;
    adts_header[2] = ((ctx->profile) << 6) + (freq_idx << 2) + (chanCfg >> 2);
    adts_header[3] = (((chanCfg & 3) << 6) + (frame_length  >> 11));
    adts_header[4] = ((frame_length & 0x7FF) >> 3);
    adts_header[5] = (((frame_length & 7) << 5) + 0x1F);
    adts_header[6] = 0xFC;
}


/**
 * Encode one frame worth of audio to the output file.
 * @param      frame                 Samples to be encoded
 * @param      output_format_context Format context of the output file
 * @param      output_codec_context  Codec context of the output file
 * @param[out] data_present          Indicates whether data has been
 *                                   encoded
 * @return Error code (0 if successful)
 */
int TranscodeAudio::encode_audio_frame(AVFrame *frame,
                              AVCodecContext *output_codec_context,
                              int *data_present)
{
    /* Packet used for temporary storage. */
    AVPacket *output_packet;
    int error;

    error = init_packet(&output_packet);
    if (error < 0)
        return error;

    /* Set a timestamp based on the sample rate for the container. */
    if (frame) {
        frame->pts = _pts;
        _pts += frame->nb_samples;
    }

    *data_present = 0;
    do {
        /* Send the audio frame stored in the temporary packet to the encoder.
        * The output audio stream encoder is used to do this. */
        error = avcodec_send_frame(output_codec_context, frame);
        /* Check for errors, but proceed with fetching encoded samples if the
        *  encoder signals that it has nothing more to encode. */
        if (error < 0 && error != AVERROR_EOF) {
            fprintf(stderr, "Could not send packet for encoding (error '%s')\n",
                    getAvError(error).c_str());
            break;
        }

        /* Receive one encoded frame from the encoder. */
        error = avcodec_receive_packet(output_codec_context, output_packet);
        /* If the encoder asks for more data to be able to provide an
        * encoded frame, return indicating that no data is present. */
        if (error == AVERROR(EAGAIN)) {
            error = 0;
            break;
        /* If the last frame has been encoded, stop encoding. */
        } else if (error == AVERROR_EOF) {
            error = 0;
            break;
        } else if (error < 0) {
            fprintf(stderr, "Could not encode frame (error '%s')\n",
                    getAvError(error).c_str());
            break;
        /* Default case: Return encoded data. */
        } else {
            *data_present = 1;
            logInfo << "Write packet pts: " << output_packet->pts << ", size: " << output_packet->size;


            StreamBuffer::Ptr buffer = make_shared<StreamBuffer>(output_packet->size + 7);
            auto bufferData = buffer->data();
            
            uint8_t header[7] = {0};
            getADTSHeader(output_codec_context, header, output_packet->size);

            memcpy(bufferData, header, 7);
            memcpy(bufferData + 7, (char*)output_packet->data, output_packet->size);
            onPacket(buffer);
        }
    } while (0);

    av_packet_free(&output_packet);
    return error;
}

/**
 * Load one audio frame from the FIFO buffer, encode and write it to the
 * output file.
 * @param fifo                  Buffer used for temporary storage
 * @param output_format_context Format context of the output file
 * @param output_codec_context  Codec context of the output file
 * @return Error code (0 if successful)
 */
int TranscodeAudio::load_encode_and_write(AVAudioFifo *fifo,
                                 AVCodecContext *output_codec_context)
{
    /* Temporary storage of the output samples of the frame written to the file. */
    AVFrame *output_frame;
    /* Use the maximum number of possible samples per frame.
     * If there is less than the maximum possible frame size in the FIFO
     * buffer use this number. Otherwise, use the maximum possible frame size. */
    const int frame_size = FFMIN(av_audio_fifo_size(fifo),
                                 output_codec_context->frame_size);
    int data_written;

    /* Initialize temporary storage for one output frame. */
    if (init_output_frame(&output_frame, output_codec_context, frame_size))
        return AVERROR_EXIT;

    /* Read as many samples from the FIFO buffer as required to fill the frame.
     * The samples are stored in the frame temporarily. */
    if (av_audio_fifo_read(fifo, (void **)output_frame->data, frame_size) < frame_size) {
        fprintf(stderr, "Could not read data from FIFO\n");
        av_frame_free(&output_frame);
        return AVERROR_EXIT;
    }

    /* Encode one frame worth of audio samples. */
    if (encode_audio_frame(output_frame,
                           output_codec_context, &data_written)) {
        av_frame_free(&output_frame);
        return AVERROR_EXIT;
    }
    av_frame_free(&output_frame);
    return 0;
}

int TranscodeAudio::inputFrame(const FrameBuffer::Ptr& frame)
{
    /* Loop as long as we have input samples to read or output samples
     * to write; abort as soon as we have neither. */
    uint8_t *data = (uint8_t*)frame->data();
    size_t   data_size = frame->size();
    int ret;
    int finished;
    const int output_frame_size = _enCodecCtx->frame_size;

    while (data_size > 0) {
        if (_deAudioCodecId != AV_CODEC_ID_PCM_ALAW) {
            ret = av_parser_parse2(_deParser, _deCodecCtx, &_dePkt->data, &_dePkt->size,
                                    data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                logError << "Error while parsing";
                return ret;
            }
        } else {
            _dePkt->data = data;
            _dePkt->size = data_size;

            ret = data_size;
        }
        data      += ret;
        data_size -= ret;

        if (_dePkt->size == 0) {
            break ;
        }

        if (read_decode_convert_and_store(_dePkt, _fifo,
                                              _deCodecCtx,
                                              _enCodecCtx,
                                              _resampleContext, &finished))
            break;

        /* If we have enough samples for the encoder, we encode them.
         * At the end of the file, we pass the remaining samples to
         * the encoder. */
        while (av_audio_fifo_size(_fifo) >= output_frame_size ||
               (finished && av_audio_fifo_size(_fifo) > 0))
            /* Take one frame worth of audio samples from the FIFO buffer,
             * encode it and write it to the output file. */
            if (ret = load_encode_and_write(_fifo,
                                      _enCodecCtx))
                return ret;
    }

    return ret;
}

void TranscodeAudio::setOnPacket(const function<void(const StreamBuffer::Ptr& packet)>& cb)
{
    _onPacket = cb;
}

void TranscodeAudio::onPacket(const StreamBuffer::Ptr& packet)
{
    if (_onPacket) {
        _onPacket(packet);
    }
}