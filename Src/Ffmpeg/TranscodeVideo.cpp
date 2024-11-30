#ifdef ENABLE_FFMPEG

#include "TranscodeVideo.h"
#include "Log/Logger.h"

using namespace std;

// static string getAvError(int errnum)
// {
//     char buffer[AV_ERROR_MAX_STRING_SIZE] = {0};
//     av_err2str_cpp(errnum, buffer, AV_ERROR_MAX_STRING_SIZE);

//     return string(buffer);
// }

static string ffmpeg_err(int errnum) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return errbuf;
}

TranscodeVideo::TranscodeVideo(const VideoEncodeOption& option, AVCodecID deVideoCodecId)
    :_option(option)
    ,_deVideoCodecId(deVideoCodecId)
{}

TranscodeVideo::~TranscodeVideo()
{
    /* flush the decoder */
    decode(_deCodecCtx, _deFrame, NULL);

    av_parser_close(_deParser);
    avcodec_free_context(&_deCodecCtx);
    avcodec_free_context(&_enCodecCtx);
    av_frame_free(&_deFrame);
    av_packet_free(&_pkt);
    av_packet_free(&_enPkt);
}

void TranscodeVideo::initEncode(AVCodecContext *dec_ctx)
{
    /* find the mpeg1video encoder */
    const AVCodec *en_codec;
    // char* codec_name = "libx264";
    en_codec = avcodec_find_encoder_by_name(_option.codec_.data());
    if (!en_codec) {
        logError << "codec not found: " << _option.codec_;
        return ;
    }

    _enCodecCtx = avcodec_alloc_context3(en_codec);
    if (!_enCodecCtx) {
        logError << "Could not allocate video codec context";
        return ;
    }

    _enPkt = av_packet_alloc();
    if (!_enPkt)
        return ;

    /* put sample parameters */
    // en_c->bit_rate = 2000000;
    /* resolution must be a multiple of two */
    _enCodecCtx->width = dec_ctx->width;
    _enCodecCtx->height = dec_ctx->height;
    /* frames per second */
    _enCodecCtx->time_base = (AVRational){1, 25};
    _enCodecCtx->framerate = (AVRational){25, 1};

    int bitrate = 1000000;
    _enCodecCtx->bit_rate = bitrate;
    _enCodecCtx->rc_buffer_size = bitrate;
    _enCodecCtx->rc_max_rate = bitrate;
    _enCodecCtx->rc_min_rate = bitrate;
    _enCodecCtx->rc_initial_buffer_occupancy = bitrate * 3 / 4;

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    _enCodecCtx->gop_size = 25;
    _enCodecCtx->max_b_frames = 0;
    _enCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    // if (en_codec->id == AV_CODEC_ID_H264)
    //     av_opt_set(en_c->priv_data, "preset", "slow", 0);

    /* open it */
    int ret = avcodec_open2(_enCodecCtx, en_codec, NULL);
    if (ret < 0) {
        logError << "Could not open codec: " << ffmpeg_err(ret);
        return ;
    }
}

void TranscodeVideo::setBitrate(int bitrate)
{
    _enCodecCtx->bit_rate = bitrate;
    _enCodecCtx->rc_buffer_size = bitrate;
    _enCodecCtx->rc_max_rate = bitrate;
    _enCodecCtx->rc_min_rate = bitrate;
    _enCodecCtx->rc_initial_buffer_occupancy = bitrate * 3 / 4;
    // _enCodecCtx->rc_max_available_vbv_use = 100;
}

void TranscodeVideo::encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt)
{
    /* send the frame to the encoder */
    // if (frame) {
    //     logInfo << "Send frame: " << frame->pts;
    // }

    frame->pts = _index++;
    // frame->dts = _index++;
    int ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        logError << "Error sending a frame for encoding";
        return ;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            logError << "Error during encoding";
            return ;
        }

        // logInfo << "Write packet pts: " << pkt->pts << ", size: " << pkt->size;
        StreamBuffer::Ptr buffer = make_shared<StreamBuffer>(pkt->size);
        buffer->assign((char*)pkt->data, pkt->size);
        onPacket(buffer);

        av_packet_unref(pkt);
    }
}

void TranscodeVideo::decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt)
{
    int ret;

    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        return ;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            return ;
        }

        // logInfo << "saving frame: " << dec_ctx->frame_num;
        fflush(stdout);

        if (!_bInitEncode) {
            _bInitEncode = 1;
            initEncode(dec_ctx);
        }

        if (!_enCodecCtx || !_enPkt) {
            logError << "init encode error";
            return ;
        }

        /* the picture is allocated by the decoder. no need to
           free it */
        // snprintf(buf, sizeof(buf), "%s-%"PRId64, filename, dec_ctx->frame_num);
        encode(_enCodecCtx, frame, _enPkt);
    }
}

void TranscodeVideo::initDecode()
{
    _pkt = av_packet_alloc();
    if (!_pkt)
        return ;
        
    _deCodec = avcodec_find_decoder(_deVideoCodecId);
    if (!_deCodec) {
        logError << "Codec not found";
        return ;
    }

    _deParser = av_parser_init(_deCodec->id);
    if (!_deParser) {
        logError << "parser not found";
        return ;
    }

    _deCodecCtx = avcodec_alloc_context3(_deCodec);
    if (!_deCodecCtx) {
        logError << "Could not allocate video codec context";
        return ;
    }

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */

    /* open it */
    if (avcodec_open2(_deCodecCtx, _deCodec, NULL) < 0) {
        logError << "Could not open codec";
        return ;
    }

    _deFrame = av_frame_alloc();
    if (!_deFrame) {
        logError << "Could not allocate video frame";
        return ;
    }
}

int TranscodeVideo::inputFrame(const FrameBuffer::Ptr& frame)
{
    uint8_t *data = (uint8_t*)frame->data();
    size_t   data_size = frame->size();
    int ret;

    /* find the MPEG-1 video decoder */
    
    // logInfo << "TranscodeVideo::inputFrame: " << data_size << " bytes";
    bool eof = !data_size;

    auto pkt = av_packet_alloc();
    pkt->data = (uint8_t *) data;
    pkt->size = data_size;
    pkt->dts = frame->dts();
    pkt->pts = frame->pts();
    if (frame->keyFrame()) {
        pkt->flags |= AV_PKT_FLAG_KEY;
    }

    decode(_deCodecCtx, _deFrame, pkt);

    av_packet_free(&pkt);

    /* use the parser to split the data into frames */
    // while (data_size > 0 || eof) {
    //     ret = av_parser_parse2(_deParser, _deCodecCtx, &_pkt->data, &_pkt->size,
    //                             data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
    //     if (ret < 0) {
    //         logError << "Error while parsing";
    //         return ret;
    //     }
    //     data      += ret;
    //     data_size -= ret;

    //     if (_pkt->size)
    //         decode(_deCodecCtx, _deFrame, _pkt);
    // }

    return 0;
}

void TranscodeVideo::setOnPacket(const function<void(const StreamBuffer::Ptr& packet)>& cb)
{
    _onPacket = cb;
}

void TranscodeVideo::onPacket(const StreamBuffer::Ptr& packet)
{
    if (_onPacket) {
        _onPacket(packet);
    }
}

#endif