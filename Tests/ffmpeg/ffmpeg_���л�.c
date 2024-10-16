#define __STDC_CONSTANT_MACROS
#include <stdio.h>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

static AVFilterContext* buffer_ctx1;
static AVFilterContext* buffer_ctx2;
static AVFilterContext* buffersink_ctx;
static AVFormatContext* fmt_ctx1;
static AVFormatContext* fmt_ctx2;
static AVFormatContext* out_fmt_ctx;
static AVCodecContext* codec_ctx1;
static AVCodecContext* codec_ctx2;
static AVCodecContext* codec_ctx_video;
static AVCodecContext* out_codec_ctx;
static int video_stream_index1;
static int video_stream_index2;
static int audio_stream_index;

#define MAX_QUEUE_SIZE 5
#define QUEUE_NAME1 "queue1"
#define QUEUE_NAME2 "queue2"

static int file_finished = 0;
static char* outfile;
static char err_buf[512];

struct FrameQueue
{
    std::queue<AVFrame*> q_frame;
    std::mutex _mutex;
    std::condition_variable _cond;
    std::string name;
};

/**
 * @brief 初始化过滤器
 */
static void initFilter()
{
    int ret;
    AVFilterGraph* filter_graph;
    AVFilterInOut* inputs = NULL;
    AVFilterInOut* outputs = NULL;
    char filter_args[1024] = {0};

    filter_graph = avfilter_graph_alloc();
    if(!filter_graph)
    {
        fprintf(stderr, "error: avfilter_graph_alloc\n");
        exit(0);
    }

    snprintf(filter_args, sizeof(filter_args),
             "buffer=video_size=%dx%d:pix_fmt=%d:time_base=1/25:pixel_aspect=%d/"
             "%d[main];"  // Parsed_buffer_0
             "buffer=video_size=%dx%d:pix_fmt=%d:time_base=1/25:pixel_aspect=%d/"
             "%d[logo];"                              // Parsed_bufer_1
             "[logo]scale=iw/2:ih/2[small];"          // Parsed_scaled_2
             "[main][small]overlay=W/2:H/2[result];"  // Parsed_overlay_3
             "[result]buffersink",                    // Parsed_buffer_sink_4
             codec_ctx1->width, codec_ctx1->height, codec_ctx1->pix_fmt, codec_ctx1->sample_aspect_ratio.num,
             codec_ctx1->sample_aspect_ratio.den, codec_ctx2->width, codec_ctx2->height, codec_ctx2->pix_fmt,
             codec_ctx2->sample_aspect_ratio.num, codec_ctx2->sample_aspect_ratio.den);

    ret = avfilter_graph_parse2(filter_graph, filter_args, &inputs, &outputs);
    if(ret < 0)
    {
        av_strerror(ret, err_buf, sizeof(err_buf));
        fprintf(stderr, "error:avfilter_graph_parse2 %s\n", err_buf);
        exit(0);
    }

    ret = avfilter_graph_config(filter_graph, NULL);
    if(ret < 0)
    {
        av_strerror(ret, err_buf, sizeof(err_buf));
        fprintf(stderr, "error:avfilter_graph_config %s\n", err_buf);
        exit(0);
    }

    buffer_ctx1 = avfilter_graph_get_filter(filter_graph, "Parsed_buffer_0");
    buffer_ctx2 = avfilter_graph_get_filter(filter_graph, "Parsed_buffer_1");
    buffersink_ctx = avfilter_graph_get_filter(filter_graph, "Parsed_buffersink_4");
}

/**
 * @brief 初始化解码器
 */
static int initDecoder(const char* filename, AVFormatContext** fmt_ctx, AVCodecContext** codec_ctx,
                       int* video_stream_index)
{
    int ret;
    AVFormatContext* _fmt_ctx = NULL;
    AVCodecContext* _codec_ctx = NULL;
    int _video_stream_index = -1;
    ret = avformat_open_input(&_fmt_ctx, filename, NULL, NULL);
    if(ret < 0)
    {
        av_strerror(ret, err_buf, sizeof(err_buf));
        fprintf(stderr, "error:avformat_open_input %s\n", err_buf);
        exit(0);
    }
    ret = avformat_find_stream_info(_fmt_ctx, NULL);
    if(ret < 0)
    {
        av_strerror(ret, err_buf, sizeof(err_buf));
        fprintf(stderr, "error:avformat_find_stream_info %s\n", err_buf);
        avformat_close_input(&_fmt_ctx);
        exit(0);
    }
    _video_stream_index = av_find_best_stream(_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(_video_stream_index < 0)
    {
        fprintf(stderr, "error:cant find video stream in file %s\n", filename);
        avformat_close_input(&_fmt_ctx);
        exit(0);
    }
    AVCodec* codec = avcodec_find_decoder(_fmt_ctx->streams[_video_stream_index]->codecpar->codec_id);
    if(!codec)
    {
        fprintf(stderr, "error:cannt find video decoder for file %s\n", filename);
        avformat_close_input(&_fmt_ctx);
        exit(0);
    }
    _codec_ctx = avcodec_alloc_context3(codec);
    if(!_codec_ctx)
    {
        fprintf(stderr, "error:alloc codeccontext failed\n");
        avformat_close_input(&_fmt_ctx);
        exit(0);
    }
    ret = avcodec_parameters_to_context(_codec_ctx, _fmt_ctx->streams[_video_stream_index]->codecpar);
    if(ret < 0)
    {
        av_strerror(ret, err_buf, sizeof(err_buf));
        fprintf(stderr, "error:avcodec_parameters_to_context %s\n", err_buf);
        avcodec_free_context(codec_ctx);
        avformat_close_input(&_fmt_ctx);
        exit(0);
    }
    ret = avcodec_open2(_codec_ctx, codec, NULL);
    if(ret < 0)
    {
        av_strerror(ret, err_buf, sizeof(err_buf));
        fprintf(stderr, "error:avcodec_open2 %s\n", err_buf);
        avcodec_free_context(&_codec_ctx);
        avformat_close_input(&_fmt_ctx);
        exit(0);
    }
    audio_stream_index = av_find_best_stream(_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(audio_stream_index < 0)
    {
        fprintf(stderr, "error:cant find audio stream in file %s\n", filename);
        avcodec_free_context(&_codec_ctx);
        avformat_close_input(&_fmt_ctx);
        exit(0);
    }
    *fmt_ctx = _fmt_ctx;
    *codec_ctx = _codec_ctx;
    *video_stream_index = _video_stream_index;
    return ret;
}

/**
 * @brief 解码视频
 */
static void decodeVideo(AVFormatContext* fmt_ctx, AVCodecContext* codec_ctx, FrameQueue* queue,
                        int* video_stream_index)
{
    fprintf(stderr, "decode start : %s\n", queue->name.c_str());
    int ret;
    AVPacket* pkt = av_packet_alloc();
    av_init_packet(pkt);
    while(1)
    {
        ret = av_read_frame(fmt_ctx, pkt);
        if(ret < 0)   // error or end of file
        {
            av_strerror(ret, err_buf, sizeof(err_buf));
            fprintf(stderr, "error:av_read_frame %s\n", err_buf);
            file_finished = 1;
            break;
        }
        if(pkt->stream_index == audio_stream_index)
        {
            if(queue->name.compare(QUEUE_NAME1) == 0)
            {
                av_packet_rescale_ts(pkt, fmt_ctx1->streams[audio_stream_index]->time_base,
                                     out_fmt_ctx->streams[audio_stream_index]->time_base);
                //写入音频pkt
                ret = av_interleaved_write_frame(out_fmt_ctx, pkt);
            }
            av_packet_unref(pkt);
            continue;
        }
        ret = avcodec_send_packet(codec_ctx, pkt);
        if(ret == 0 || ret == AVERROR(EAGAIN))   // send success or need read again
        {
            while(1)
            {
                AVFrame* frame = av_frame_alloc();
                ret = avcodec_receive_frame(codec_ctx, frame);
                if(ret == AVERROR(EAGAIN))
                {
                    av_frame_free(&frame);
                    break;
                }
                else if(ret == AVERROR_EOF)
                {
                    av_frame_free(&frame);
                    return;
                }
                else if(ret < 0)
                {
                    av_strerror(ret, err_buf, sizeof(err_buf));
                    av_frame_free(&frame);
                    break;
                }
                else
                {
                    std::unique_lock<std::mutex> lock(queue->_mutex);
                    while(queue->q_frame.size() >= MAX_QUEUE_SIZE && !file_finished)
                    {
                        fprintf(stdout, "queue wait : %s\n", queue->name.c_str());
                        queue->_cond.wait(lock);
                    }
                    if(file_finished)
                    {
                        return;
                    }
                    fprintf(stdout, "push : %s\n", queue->name.c_str());
                    queue->q_frame.push(frame);
                    queue->_cond.notify_all();
                }
            }
        }
        else
        {
            av_strerror(ret, err_buf, sizeof(err_buf));
            fprintf(stderr, "error:send packet %s\n", err_buf);
            break;
        }
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
    fprintf(stdout, "-----------------decode stop : %s-----------------\n", queue->name.c_str());
}

/**
 * @brief 初始化编码器
 */
static int initEncoder(AVFormatContext** fmt_ctx, AVCodecContext** codec_ctx, const char* file_name)
{
    int ret;
    AVFormatContext* _fmt_ctx = NULL;
    AVCodecContext* _codec_ctx = NULL;

    AVCodec* codec = avcodec_find_encoder(fmt_ctx1->streams[video_stream_index1]->codecpar->codec_id);
    if(!codec)
    {
        fprintf(stderr, "error:avcodec_find_encoder\n");
        exit(0);
    }
    ret = avformat_alloc_output_context2(&_fmt_ctx, NULL, NULL, file_name);
    if(ret < 0)
    {
        av_strerror(ret, err_buf, sizeof(err_buf));
        fprintf(stderr, "error:avformat_alloc_output_context2 %s\n", err_buf);
        exit(0);
    }
    _codec_ctx = avcodec_alloc_context3(codec);
    if(!_codec_ctx)
    {
        fprintf(stderr, "error:avcodec_alloc_context3\n");
        exit(0);
    }
    _codec_ctx->width = fmt_ctx1->streams[video_stream_index1]->codecpar->width;
    _codec_ctx->height = fmt_ctx1->streams[video_stream_index1]->codecpar->height;
    _codec_ctx->pix_fmt = (AVPixelFormat)fmt_ctx1->streams[video_stream_index1]->codecpar->format;
    _codec_ctx->time_base = (AVRational)
    {
        1, 25
    };
    //    _codec_ctx->bit_rate = 100 * 1000;
    //    _codec_ctx->framerate = (AVRational){25, 1};
    //    _codec_ctx->gop_size = 10;
    //    _codec_ctx->max_b_frames = 1;

    //增加视频流
    AVStream* video_stream = avformat_new_stream(_fmt_ctx, codec);
    if(!video_stream)
    {
        avformat_free_context(_fmt_ctx);
        avcodec_free_context(&_codec_ctx);
        fprintf(stderr, "error:avformat_new_stream\n");
        exit(0);
    }
    ret = avcodec_parameters_from_context(video_stream->codecpar, _codec_ctx);
    if(ret < 0)
    {
        avformat_free_context(_fmt_ctx);
        avcodec_free_context(&_codec_ctx);
        av_strerror(ret, err_buf, sizeof(err_buf));
        fprintf(stderr, "error:avcodec_parameters_from_context %s\n", err_buf);
        exit(0);
    }
    //    video_stream->id = _fmt_ctx->nb_streams - 1;
    //    video_stream->time_base = _codec_ctx->time_base;
    ret = avcodec_open2(_codec_ctx, codec, NULL);
    if(ret < 0)
    {
        avformat_free_context(_fmt_ctx);
        avcodec_free_context(&_codec_ctx);
        av_strerror(ret, err_buf, sizeof(err_buf));
        fprintf(stderr, "error:avcodec_open2 %s\n", err_buf);
        exit(0);
    }

    //增加音频流
    AVStream* audio_stream = avformat_new_stream(_fmt_ctx, NULL);
    if(!audio_stream)
    {
        avformat_free_context(_fmt_ctx);
        avcodec_free_context(&_codec_ctx);
        avcodec_close(_codec_ctx);
        fprintf(stderr, "error:avformat_new_stream\n");
        exit(0);
    }
    ret = avcodec_parameters_copy(audio_stream->codecpar, fmt_ctx1->streams[audio_stream_index]->codecpar);
    if(ret < 0)
    {
        avformat_free_context(_fmt_ctx);
        avcodec_free_context(&_codec_ctx);
        avcodec_close(_codec_ctx);
        av_strerror(ret, err_buf, sizeof(err_buf));
        fprintf(stderr, "error:avcodec_parameters_copy %s\n", err_buf);
        exit(0);
    }
    //    audio_stream->time_base = (AVRational){1, audio_stream->codecpar->sample_rate};
    //    audio_stream->id = _fmt_ctx->nb_streams - 1;

    ret = avio_open(&(_fmt_ctx->pb), outfile, AVIO_FLAG_WRITE);
    if(ret < 0)
    {
        avformat_free_context(_fmt_ctx);
        avcodec_free_context(&_codec_ctx);
        avcodec_close(_codec_ctx);
        av_strerror(ret, err_buf, sizeof(err_buf));
        fprintf(stderr, "error:avio_open %s\n", err_buf);
        exit(0);
    }
    audio_stream->codecpar->codec_tag = 0;
    ret = avformat_write_header(_fmt_ctx, NULL);
    if(ret < 0)
    {
        avformat_free_context(_fmt_ctx);
        avcodec_free_context(&_codec_ctx);
        avcodec_close(_codec_ctx);
        av_strerror(ret, err_buf, sizeof(err_buf));
        fprintf(stderr, "error:avformat_write_header %s\n", err_buf);
        exit(0);
    }
    *fmt_ctx = _fmt_ctx;
    *codec_ctx = _codec_ctx;
    return ret;
}

/**
 * @brief 编码视频
 */
static void encodeVideo(AVFormatContext* fmt_ctx, AVCodecContext* codec_ctx, AVFrame* frame, FILE* fp)
{
    int ret;
    ret = avcodec_send_frame(codec_ctx, frame);
    AVPacket* pkt = av_packet_alloc();
    av_init_packet(pkt);
    while(ret == 0)
    {
        ret = avcodec_receive_packet(codec_ctx, pkt);
        if(ret == 0)
        {
            pkt->stream_index = 0;
            av_packet_rescale_ts(pkt, fmt_ctx1->streams[video_stream_index1]->time_base,
                                 fmt_ctx->streams[pkt->stream_index]->time_base);
            //            fprintf(stdout, "video dts : %d\n", pkt->dts);
            //写入视频pkt
            ret = av_interleaved_write_frame(fmt_ctx, pkt);
            if(ret < 0)
            {
                av_strerror(ret, err_buf, sizeof(err_buf));
                fprintf(stderr, "error:av_interleaved_write_frame : %s\n", err_buf);
                exit(0);
            }
        }
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
}

/**
 * @brief 保存视频
 */
static void saveFrame(AVFrame* frame, FILE* out)
{
    int width = frame->width;
    int height = frame->height;
    // save Y
    for(int i = 0; i < height; i++)
    {
        fwrite(frame->data[0] + i * frame->linesize[0], 1, frame->width, out);
    }
    // save U
    for(int i = 0; i < height / 2; i++)
    {
        fwrite(frame->data[1] + i * frame->linesize[1], 1, frame->width / 2, out);
    }
    // save V
    for(int i = 0; i < height / 2; i++)
    {
        fwrite(frame->data[2] + i * frame->linesize[2], 1, frame->width / 2, out);
    }
}

int main(int argc, char** argv)
{
    int ret;
    char* infile1, *infile2;
    FILE* fp1 = NULL, *fp2 = NULL;
    FILE* fp = NULL;
    if(argc != 3)
    {
        fprintf(stderr, "arg count error\n");
        exit(0);
    }
    infile1 = argv[1];
    infile2 = infile1;
    outfile = argv[2];
    fp1 = fopen(infile1, "rb");
    if(!fp1)
    {
        fprintf(stderr, "infile %s open failed\n", infile1);
        exit(0);
    }
    fp2 = fopen(infile2, "rb");
    if(!fp2)
    {
        fprintf(stderr, "infile %s open failed\n", infile2);
        exit(0);
    }
    fp = fopen(outfile, "wb");
    if(!fp)
    {
        fprintf(stderr, "outfile %s open failed\n", outfile);
        exit(0);
    }
    initDecoder(infile1, &fmt_ctx1, &codec_ctx1, &video_stream_index1);
    initDecoder(infile2, &fmt_ctx2, &codec_ctx2, &video_stream_index2);
    initEncoder(&out_fmt_ctx, &out_codec_ctx, outfile);
    FrameQueue src_queue1;  // queue1用于存放从第一个视频读取到的pkt
    src_queue1.name = QUEUE_NAME1;
    FrameQueue src_queue2;  // queue2用于存放从第二个视频读取到的pkt
    src_queue2.name = QUEUE_NAME2;
    std::thread decode_thread1(decodeVideo, fmt_ctx1, codec_ctx1, &src_queue1,
                               &video_stream_index1);  //读第一个视频的线程1
    std::thread decode_thread2(decodeVideo, fmt_ctx2, codec_ctx2, &src_queue2,
                               &video_stream_index2);  //读第二个视频的线程2
    initFilter();
    while(1)
    {
        //从src_queue1中读取一个pkt
        std::unique_lock<std::mutex> lock1(src_queue1._mutex);
        while(src_queue1.q_frame.empty() && !file_finished)
        {
            fprintf(stdout, "src_queue1 wait\n");
            src_queue1._cond.wait(lock1);
        }
        if(file_finished && src_queue1.q_frame.size() == 0)
        {
            break;
        }
        AVFrame* src_frame1 = src_queue1.q_frame.front();
        lock1.unlock();
        //从src_queue2中读取一个pkt
        std::unique_lock<std::mutex> lock2(src_queue2._mutex);
        while(src_queue2.q_frame.empty() && !file_finished)
        {
            fprintf(stdout, "src_queue2 wait\n");
            src_queue2._cond.wait(lock2);
        }
        if(file_finished && src_queue2.q_frame.size() == 0)
        {
            break;
        }
        AVFrame* src_frame2 = src_queue2.q_frame.front();
        lock2.unlock();
        fprintf(stdout, "add buffersrc1\n");
        ret = av_buffersrc_add_frame(buffer_ctx1, src_frame1);
        if(ret < 0)
        {
            av_strerror(ret, err_buf, sizeof(err_buf));
            fprintf(stderr, "buffersrc1 add frame error %s\n", err_buf);
            break;
        }
        fprintf(stdout, "add buffersrc2\n");
        ret = av_buffersrc_add_frame(buffer_ctx2, src_frame2);
        if(ret < 0)
        {
            av_strerror(ret, err_buf, sizeof(err_buf));
            fprintf(stderr, "buffersrc2 add frame error %s\n", err_buf);
            break;
        }
        AVFrame* out_frame = av_frame_alloc();
        fprintf(stdout, "get buffersink\n");
        ret = av_buffersink_get_frame(buffersink_ctx, out_frame);
        if(ret < 0)
        {
            av_strerror(ret, err_buf, sizeof(err_buf));
            fprintf(stderr, "buffersink get frame error %s\n", err_buf);
            av_frame_free(&out_frame);
            break;
        }
        //    saveFrame(out_frame, fp);
        encodeVideo(out_fmt_ctx, out_codec_ctx, out_frame, fp);
        av_frame_unref(out_frame);
        av_frame_free(&src_frame1);
        av_frame_free(&src_frame2);
        lock1.lock();
        src_queue1.q_frame.pop();
        src_queue1._cond.notify_one();
        lock2.lock();
        src_queue2.q_frame.pop();
        src_queue2._cond.notify_one();
    }
    // flush encoder
    encodeVideo(out_fmt_ctx, out_codec_ctx, NULL, fp);
    av_write_trailer(out_fmt_ctx);
    decode_thread1.join();
    decode_thread2.join();
    fprintf(stdout, "success\n");
    return 0;
}