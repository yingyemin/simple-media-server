#ifndef TranscodeVideo_H
#define TranscodeVideo_H

#include "Common/Frame.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <string>

class VideoEncodeOption
{
public:
    std::string codec_;
    int width_;
    int height_;
    int bitrate_;
    int gop_;
    int maxBFrame_;
    AVPixelFormat fixFmt_;
};

class TranscodeVideo
{
public:
    TranscodeVideo(const VideoEncodeOption& option, AVCodecID deVideoCodecId);
    ~TranscodeVideo();

public:
    void initEncode(AVCodecContext *dec_ctx);
    void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt);
    void initDecode();
    void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt);
    int inputFrame(const FrameBuffer::Ptr& frame);

    void setOnPacket(const function<void(const StreamBuffer::Ptr& packet)>& cb);
    void onPacket(const StreamBuffer::Ptr& packet);

private:
    VideoEncodeOption _option;
    AVCodecContext *_enCodecCtx= NULL;
    AVPacket *_enPkt;
    int _bInitEncode = 0;

    const AVCodec *_deCodec;
    AVCodecParserContext *_deParser;
    AVCodecContext *_deCodecCtx= NULL;
    AVFrame *_deFrame;
    AVPacket *_pkt;

    AVCodecID _deVideoCodecId;

    function<void(const StreamBuffer::Ptr& packet)> _onPacket;
};

#endif