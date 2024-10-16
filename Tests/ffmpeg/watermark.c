#include <iostream>
#include <memory>
#include <array>
#include <string>
#include <sstream>
#include <functional>

extern "C"
{
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/frame.h>
}

#define WIDTH_MAIN 720
#define HEIGHT_MAIN 576

//////////////////////////////////////////////////////////////////////////
int load_image(std::shared_ptr<AVFrame> &apAVFrame,
               const std::string &strFilepath,
               std::string &out_wstrError)
{
    out_wstrError.clear();

    AVFormatContext *pFormatCtx = nullptr;

    int ret = 0;
    if ((ret = avformat_open_input(&pFormatCtx, strFilepath.c_str(), nullptr, nullptr)) < 0) {
        out_wstrError = "Failed to open input file '" + strFilepath + "'";
        return ret;
    }

    const std::shared_ptr<AVFormatContext> apFormatCtx =
        {
            pFormatCtx,
            [](AVFormatContext *pFmtCtx)
            {
                if (pFmtCtx) { avformat_close_input(&pFmtCtx); }
            }
        };

    if ((ret = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0)) < 0)
    {
        out_wstrError = "Failed to find video stream '" + strFilepath + "'";
        return ret;
    }

    pFormatCtx = nullptr;

    AVCodecParameters *par = apFormatCtx->streams[ret]->codecpar;
    AVCodec *pCodec = avcodec_find_decoder(par->codec_id);
    if (!pCodec)
    {
        out_wstrError = "Failed to find codec: " + ret;
        return AVERROR(EINVAL);
    }

    const std::shared_ptr<AVCodecContext> apCodecCtx =
        {
            avcodec_alloc_context3(pCodec),
            [](AVCodecContext *pCodecCtx)
            {
                if (pCodecCtx) { avcodec_free_context(&pCodecCtx); }
            }
        };

    ret = avcodec_parameters_to_context(apCodecCtx.get(), par);
    if (ret < 0)
    {
        out_wstrError = "Failed to copy codec parameters to decoder context: " + ret;
        return ret;
    }

    apCodecCtx->skip_alpha = 0;
    apCodecCtx->sw_pix_fmt = AV_PIX_FMT_ARGB;

    //av_dict_set(&opt, "thread_type", "slice", 0);
    if ((ret = avcodec_open2(apCodecCtx.get(), pCodec, nullptr)) < 0)
    {
        out_wstrError = "Failed to open codec: " + ret;
        return ret;
    }

    if (!apAVFrame)
    {
        apAVFrame.reset(av_frame_alloc(),
                        [](AVFrame *pAVFrame)
                        {
                            if (pAVFrame) { av_frame_free(&pAVFrame); }
                        });

        av_frame_unref(apAVFrame.get());
    }

    AVPacket pkt={};
    av_init_packet(&pkt);

    ret = av_read_frame(apFormatCtx.get(), &pkt);
    if (ret < 0) {
        out_wstrError = "Failed to read frame from file: " + ret;
        return ret;
    }

    ret = avcodec_send_packet(apCodecCtx.get(), &pkt);
    if (ret < 0)
    {
        out_wstrError = "Failed to decode image from file: " + ret;
        return ret;
    }

    ret = avcodec_receive_frame(apCodecCtx.get(), apAVFrame.get());
    if (ret < 0)
    {
        out_wstrError = "Failed to decode image from file: " + ret;
        return ret;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
int open_encoder_context(AVFormatContext *const in_pFmtCtx,
                               std::shared_ptr<AVCodecContext> &inout_apCdcCtx,
                               AVStream *&inout_pStream,
                               enum AVCodecID in_codecId,
                               const std::function<int(AVStream *, AVCodecContext *, AVDictionary *&)> &fnInitContext)
{
    inout_apCdcCtx.reset();
    inout_pStream = nullptr;

    if (!in_pFmtCtx)
    {
        std::cerr << "Pointer to format context is NULL";
        return -1;
    }

    // Find the encoder
    auto pCdc = avcodec_find_encoder(in_codecId);
    if (pCdc == nullptr)
    {
        std::cerr << "Failed to find encoder codec";
        return -1;
    }

    auto pStream = avformat_new_stream(in_pFmtCtx, nullptr);
    if (pStream == nullptr)
    {
        std::cerr << "Could not allocate elementary stream";
        return -1;
    }

    pStream->id = in_pFmtCtx->nb_streams - 1;

    // Allocate a codec context for the encoder
    std::unique_ptr<AVCodecContext, std::function<void(AVCodecContext *)>> apCdcCtx
        (
            avcodec_alloc_context3(pCdc),
            [](AVCodecContext *pCdcCtx)
            {
                // Not sure this function can take a nullptr!
                if (pCdcCtx != nullptr)
                {
                    avcodec_free_context(&pCdcCtx);
                }
            }
        );

    // Allocate a codec context for the encoder
    if (!apCdcCtx)
    {
        std::cerr << "Failed to allocate the '"
                  << av_get_media_type_string(pCdc->type)
                  << "' encoder codec context.";

        return -1;
    }

    AVDictionary *pDict = nullptr;
    if (fnInitContext)
    {
        const int r = fnInitContext(pStream, apCdcCtx.get(), pDict);
        if (r != 0)
        {
            std::cerr << "Failed to initialise encoder codeccontext using custom initialisation function.";
            return r;
        }
    }

    // Some formats want stream headers to be separate.
    if (in_pFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
    {
        apCdcCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    // Init the encoder
    {
        const int ret = avcodec_open2(apCdcCtx.get(), pCdc, &pDict);
        if (ret < 0)
        {
            std::cerr << "Failed to open '"
                      << av_get_media_type_string(apCdcCtx->codec_type)
                      << "' encoder codec: "
                      << ret;

            return ret;
        }
    }

    // Fill the parameters struct based on the values from the supplied codec context. This
    // sets the parameters in the muxer.
    {
        const int ret = avcodec_parameters_from_context(pStream->codecpar, apCdcCtx.get());
        if (ret < 0)
        {
            std::cerr << "Could not copy the encoder context stream parameters to the multiplexer";
            return ret;
        }
    }

    inout_apCdcCtx = std::shared_ptr<AVCodecContext>
        (
            apCdcCtx.release(),
            [](AVCodecContext *pCdcCtx)
            {
                // Not sure this function can take a nullptr!
                if (pCdcCtx != nullptr)
                {
                    avcodec_free_context(&pCdcCtx);
                }
            }
        );

    inout_pStream = pStream;

    return 0;
}

//////////////////////////////////////////////////////////////////////////
int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
    // rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;

    // Write the compressed frame to the media file.
    return av_interleaved_write_frame(fmt_ctx, pkt);
}

//////////////////////////////////////////////////////////////////////////
int create_filter(const std::shared_ptr<AVFilterGraph> &apFilterGraph,
                  const std::string &strFilterType,
                  const std::string &strFilterName,
                  const std::string &strArgs,
                  AVFilterContext *&out_pFilterContext)
{
    return avfilter_graph_create_filter(&out_pFilterContext,
                                        avfilter_get_by_name(strFilterType.c_str()),
                                        strFilterName.c_str(),
                                        strArgs.c_str(),
                                        nullptr,
                                        apFilterGraph.get());
}

//////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
#if 1
    ////////////////////
    // Depending on your version of FFmpeg, this may not be needed.
    av_register_all();
    avfilter_register_all();
    ////////////////////
#endif

    std::shared_ptr<AVFrame> apFrameOverlay;
    {
        std::string strError;
        const int ret = load_image(apFrameOverlay, argv[1], strError);

        if (ret != 0)
        {
            return ret;
        }
    }

    std::shared_ptr<AVFormatContext> apOc;

    // Open output context.
    {
        AVFormatContext *oc{nullptr};
        avformat_alloc_output_context2(&oc, nullptr, nullptr, argv[2]);
        if (!oc)
        {
            std::cerr << "Could not deduce output format from file extension. Aborting";
            return -1;
        }

        apOc = {oc, [](AVFormatContext *pFmtCtx) { if (pFmtCtx){ avformat_free_context(pFmtCtx); } } };
    }

    AVStream *pStream{nullptr};
    std::shared_ptr<AVCodecContext> apCdcCtx;
    {
        const int r = open_encoder_context(apOc.get(),
                                           apCdcCtx,
                                           pStream,
                                           AV_CODEC_ID_H264,
                                           [](AVStream *pStream, AVCodecContext *pCdcCtx, AVDictionary *&dict) -> int
                                           {
                                               //pCdcCtx->codec_id = codec_id;

                                               pCdcCtx->bit_rate = 4000000;

                                               /* Resolution must be a multiple of two. */
                                               pCdcCtx->width    = WIDTH_MAIN;
                                               pCdcCtx->height   = HEIGHT_MAIN;

                                               /* timebase: This is the fundamental unit of time (in seconds) in terms
                                                * of which frame timestamps are represented. For fixed-fps content,
                                                * timebase should be 1/framerate and timestamp increments should be
                                                * identical to 1. */
                                               pStream->time_base = (AVRational){ 1, 25 };

                                               pCdcCtx->time_base       = pStream->time_base;

                                               pCdcCtx->gop_size      = 1;
                                               pCdcCtx->pix_fmt       = AV_PIX_FMT_YUV422P;


                                               return 0;
                                           }
        );
    }

    const std::shared_ptr<AVFilterGraph> apFilterGraph =
        {
            avfilter_graph_alloc(),
            [](AVFilterGraph *pFilterGraph)
            {
                if (pFilterGraph)
                {
                    avfilter_graph_free(&pFilterGraph);
                }
            }
        };

    enum filter_index
    {
        FLTR_BUFFER_MAIN = 0,
        FLTR_BUFFER_OVERLAY = 1,
        FLTR_OVERLAY = 2,
        FLTR_BUFFER_SINK = 3,

        FLTR_COUNT
    };

    std::array<AVFilterContext*, FLTR_COUNT> filters{};

    std::shared_ptr<AVFrame> apFrameMain{av_frame_alloc(),
                                         [](AVFrame *pAVFrame)
                                         {
                                             if (pAVFrame != nullptr)
                                             {
                                                 av_frame_free(&pAVFrame);
                                             }
                                         }};

    apFrameMain->format = AV_PIX_FMT_YUV422P;
    apFrameMain->width = WIDTH_MAIN;
    apFrameMain->height = HEIGHT_MAIN;

    {
        const int ret = av_frame_get_buffer(apFrameMain.get(), 32);

        if (ret < 0)
        {
            std::cerr << "Failed to allocate main frame."
                      << std::endl;

            return -1;
        }
    }

    // Buffer Main
    {
        std::ostringstream oss;
        oss << "width="
            << apFrameMain->width
            << ":height="
            << apFrameMain->height
            << ":pix_fmt="
            << av_get_pix_fmt_name(static_cast<AVPixelFormat>(apFrameMain->format))
            << ":time_base=1/25";

        const int ret = create_filter(apFilterGraph,
                                      "buffer",
                                      "bufferMain",
                                      oss.str(),
                                      filters[FLTR_BUFFER_MAIN]);

        if (ret < 0)
        {
            std::cout << "Cannot create source filter 'bufferMain' (args: "
                                 << oss.str()
                                 << " ) - "
                                 << ret;
            return -1;
        }
    }

    // Buffer Overlay
    {
        std::ostringstream oss;
        oss << "width="
            << 720//apFrameOverlay->width
            << ":height="
            << 576//apFrameOverlay->height
            << ":pix_fmt="
            << av_get_pix_fmt_name(static_cast<AVPixelFormat>(apFrameOverlay->format))
            << ":time_base=1/25";

        const int ret = create_filter(apFilterGraph,
                                      "buffer",
                                      "bufferOverlay",
                                      oss.str(),
                                      filters[FLTR_BUFFER_OVERLAY]);

        if (ret < 0)
        {
            std::cout << "Cannot create source filter 'bufferOverlay' (args: "
                      << oss.str()
                      << " ) - "
                      << ret;
            return -1;
        }
    }

    // Overlay
    {
        std::ostringstream oss;
        oss << "eval=frame:format=yuv422:x=0:y=0";

        const int ret = create_filter(apFilterGraph,
                                      "overlay",
                                      "overlay",
                                      oss.str(),
                                      filters[FLTR_OVERLAY]);

        if (ret < 0)
        {
            std::cout << "Cannot create source filter 'overlay' (args: "
                      << oss.str()
                      << " ) - "
                      << ret;
            return -1;
        }
    }

    // Buffer Sink Main
    {
        const int ret = create_filter(apFilterGraph,
                                      "buffersink",
                                      "buffersink",
                                      std::string(),
                                      filters[FLTR_BUFFER_SINK]);

        if (ret < 0)
        {
            std::cout << "Cannot create source filter 'buffersink' - "
                      << ret;
            return -1;
        }
    }

    // Link BufferSrc Main to Overlay
    {
        const int ret = avfilter_link(filters[FLTR_BUFFER_MAIN], 0,
                                      filters[FLTR_OVERLAY], 0);

        if (ret < 0)
        {
            std::cerr << "Error connecting buffer main to overlay: " << ret;
            return -1;
        }
    }

    // Link BufferSrc Overlay to Overlay
    {
        const int ret = avfilter_link(filters[FLTR_BUFFER_OVERLAY], 0,
                                      filters[FLTR_OVERLAY], 1);

        if (ret < 0)
        {
            std::cerr << "Error connecting buffer overlay to overlay: " << ret;
            return -1;
        }
    }

    // Link Overlay to Buffersink
    {
        const int ret = avfilter_link(filters[FLTR_OVERLAY], 0,
                                      filters[FLTR_BUFFER_SINK], 0);

        if (ret < 0)
        {
            std::cerr << "Error connecting overlay to buffersink: " << ret;
            return -1;
        }
    }

    // Configure the graph.
    {
        const int ret = avfilter_graph_config(apFilterGraph.get(), nullptr);
        if (ret < 0)
        {
            std::cout << "Failed to configure the graph: " << ret;
            return -1;
        }
    }

    {
        /* open the output file, if needed */
        if (!(apOc->oformat->flags & AVFMT_NOFILE))
        {
            const int ret = avio_open(&apOc->pb, argv[2], AVIO_FLAG_WRITE);
            if (ret < 0) {
                std::cerr << "Could not open '" << argv[2] << "': " << ret << std::endl;
                return 1;
            }
        }
    }

    {
        // Write the stream header, if any.
        const int ret = avformat_write_header(apOc.get(), nullptr);
        if (ret < 0)
        {
            std::cerr << "Error occurred when opening output file: " << ret << std::endl;
            return -1;
        }
    }

    // Write 10 seconds worth of data (@ 25fps)
    for (std::size_t i = 0; i < 250; ++i)
    {
        // Add MAIN frame.
        {
            std::shared_ptr<AVFrame> apFrameMainTemp{av_frame_clone(apFrameMain.get()),
                                                     [](AVFrame *pAVFrame)
                                                     {
                                                         if (pAVFrame != nullptr)
                                                         {
                                                             av_frame_free(&pAVFrame);
                                                         }
                                                     }};

            apFrameMainTemp->pts = i;

            const int ret = av_buffersrc_add_frame_flags(filters[FLTR_BUFFER_MAIN],
                                                         apFrameMainTemp.get(),
                                                         AV_BUFFERSRC_FLAG_PUSH);

            if (ret < 0)
            {
                std::cerr << "Failed to add main frame to graph (index: "
                          << i
                          << "): "
                          << ret
                          << ". Aborting."
                          << std::endl;

                break;
            }
        }

        // Add OVERLAY frame.
        {
            std::shared_ptr<AVFrame> apFrameOverlayTemp{av_frame_clone(apFrameOverlay.get()),
                                                        [](AVFrame *pAVFrame)
                                                        {
                                                            if (pAVFrame != nullptr)
                                                            {
                                                                av_frame_free(&pAVFrame);
                                                            }
                                                        }};

            apFrameOverlayTemp->pts = i;

            const int ret = av_buffersrc_add_frame_flags(filters[FLTR_BUFFER_OVERLAY],
                                                         apFrameOverlayTemp.get(),
                                                         AV_BUFFERSRC_FLAG_PUSH);

            if (ret < 0)
            {
                std::cerr << "Failed to add overlay frame to graph (index: "
                          << i
                          << "): "
                          << ret
                          << ". Aborting."
                          << std::endl;
                break;
            }
        }

        const std::shared_ptr<AVFrame> apAVFrameOuptut{av_frame_alloc(),
                                                       [](AVFrame *pAVFrame)
                                                       {
                                                           if (pAVFrame != nullptr)
                                                           {
                                                               av_frame_free(&pAVFrame);
                                                           }
                                                       }};

        // Fetch frame.
        {

            const int ret = av_buffersink_get_frame(filters[FLTR_BUFFER_SINK], apAVFrameOuptut.get());
            if (ret < 0)
            {
                std::cerr << "Failed to fetch frame from graph (index: "
                          << i
                          << "): "
                          << ret
                          << ". Aborting."
                          << std::endl;
                break;
            }
        }

        // Send frame to encoder
        {
            const int ret = avcodec_send_frame(apCdcCtx.get(), apAVFrameOuptut.get());

        }

        // Receive frame from decoder
        {
            AVPacket pkt{};
            av_init_packet(&pkt);

            const int ret = avcodec_receive_packet(apCdcCtx.get(), &pkt);
            if (ret == 0) // Success - have packet
            {

                write_frame(apOc.get(), &apCdcCtx->time_base, pStream, &pkt);

                av_packet_unref(&pkt);
            }
            else if (ret == AVERROR(EAGAIN)) // No output available.
            {

            }
            else
            {
                std::cerr << "Received error getting packet from encoder: "
                          << ret
                          << ". Aborting."
                          << std::endl;

                break;
            }
        }

        std::cout << "Processed frame index "
                  << i
                  << std::endl;
    }

    // Flush the encoder
    {
        avcodec_send_frame(apCdcCtx.get(), nullptr);

        AVPacket pkt{};
        av_init_packet(&pkt);

        while (0 == avcodec_receive_packet(apCdcCtx.get(), &pkt))
        {
            write_frame(apOc.get(), &apCdcCtx->time_base, pStream, &pkt);

            av_packet_unref(&pkt);
        }
    }

    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    av_write_trailer(apOc.get());

    if (!(apOc->oformat->flags & AVFMT_NOFILE))
    {
        // Close the output file.
        avio_closep(&apOc->pb);
    }

    return 0;
}