#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavcodec/avcodec.h>

#define INBUF_SIZE 409600

AVCodecContext *en_c= NULL;
AVPacket *en_pkt;
int bInitEncode = 0;

static void initEncode(AVCodecContext *dec_ctx)
{
    /* find the mpeg1video encoder */
    const AVCodec *en_codec;
    char* codec_name = "libx264";
    en_codec = avcodec_find_encoder_by_name(codec_name);
    if (!en_codec) {
        fprintf(stderr, "Codec '%s' not found\n", codec_name);
        exit(1);
    }

    en_c = avcodec_alloc_context3(en_codec);
    if (!en_c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    en_pkt = av_packet_alloc();
    if (!en_pkt)
        exit(1);

    /* put sample parameters */
    // en_c->bit_rate = 2000000;
    /* resolution must be a multiple of two */
    en_c->width = dec_ctx->width;
    en_c->height = dec_ctx->height;
    /* frames per second */
    en_c->time_base = (AVRational){1, 25};
    en_c->framerate = (AVRational){25, 1};

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    en_c->gop_size = 25;
    en_c->max_b_frames = 0;
    en_c->pix_fmt = AV_PIX_FMT_YUV420P;

    // if (en_codec->id == AV_CODEC_ID_H264)
    //     av_opt_set(en_c->priv_data, "preset", "slow", 0);

    /* open it */
    int ret = avcodec_open2(en_c, en_codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
        exit(1);
    }
}

static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
                   const char *outfilename)
{
    int ret;
    FILE *outfile = fopen(outfilename, "ab+");

    /* send the frame to the encoder */
    if (frame)
        printf("Send frame %3"PRId64"\n", frame->pts);

    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            fclose(outfile);
            return;
        } else if (ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }

        printf("Write packet %3"PRId64" (size=%5d)\n", pkt->pts, pkt->size);
        fwrite(pkt->data, 1, pkt->size, outfile);
        av_packet_unref(pkt);
    }
    fclose(outfile);
}

static void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt,
                   const char *filename)
{
    char buf[1024];
    int ret;

    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }

        printf("saving frame %3"PRId64"\n", dec_ctx->frame_num);
        fflush(stdout);

        if (!bInitEncode) {
            bInitEncode = 1;
            initEncode(dec_ctx);
        }

        /* the picture is allocated by the decoder. no need to
           free it */
        // snprintf(buf, sizeof(buf), "%s-%"PRId64, filename, dec_ctx->frame_num);
        encode(en_c, frame, en_pkt, filename);
    }
}

int main(int argc, char **argv)
{
    const char *filename, *outfilename;
    const AVCodec *codec;
    AVCodecParserContext *parser;
    AVCodecContext *c= NULL;
    FILE *f;
    AVFrame *frame;
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data;
    size_t   data_size;
    int ret;
    int eof;
    AVPacket *pkt;

    if (argc <= 2) {
        fprintf(stderr, "Usage: %s <input file> <output file>\n"
                "And check your input file is encoded by mpeg1video please.\n", argv[0]);
        exit(0);
    }
    filename    = argv[1];
    outfilename = argv[2];

    pkt = av_packet_alloc();
    if (!pkt)
        exit(1);

    /* set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) */
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    /* find the MPEG-1 video decoder */
    codec = avcodec_find_decoder(AV_CODEC_ID_H265);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    parser = av_parser_init(codec->id);
    if (!parser) {
        fprintf(stderr, "parser not found\n");
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    do {
        /* read raw data from the input file */
        data_size = fread(inbuf, 1, INBUF_SIZE, f);
        if (ferror(f))
            break;
        eof = !data_size;

        /* use the parser to split the data into frames */
        data = inbuf;
        while (data_size > 0 || eof) {
            ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
                                   data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                fprintf(stderr, "Error while parsing\n");
                exit(1);
            }
            data      += ret;
            data_size -= ret;

            if (pkt->size)
                decode(c, frame, pkt, outfilename);
            else if (eof)
                break;
        }
    } while (!eof);

    /* flush the decoder */
    decode(c, frame, NULL, outfilename);

    fclose(f);

    av_parser_close(parser);
    avcodec_free_context(&c);
    avcodec_free_context(&en_c);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    av_packet_free(&en_pkt);

    return 0;
}

