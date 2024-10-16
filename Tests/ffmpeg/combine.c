/**
*
* 张晖 Hui Zhang
* zhanghuicuc@gmail.com
* 中国传媒大学/数字电视技术
* Communication University of China / Digital TV Technology
*
* 本程序实现了对多路视频进行合并实现分屏效果,并且可以添加不同的视频滤镜。
* 目前还不支持音频合并
*/

#include <stdio.h>
extern "C"
{
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libavutil/mathematics.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
};
#include <crtdbg.h>

/*
FIX: H.264 in some container format (FLV, MP4, MKV etc.) need
"h264_mp4toannexb" bitstream filter (BSF)
*Add SPS,PPS in front of IDR frame
*Add start code ("0,0,0,1") in front of NALU
H.264 in some container (MPEG2TS) don't need this BSF.
*/
//'1': Use H.264 Bitstream Filter   
#define USE_H264BSF 0  

/*
FIX:AAC in some container format (FLV, MP4, MKV etc.) need
"aac_adtstoasc" bitstream filter (BSF)
*/
//'1': Use AAC Bitstream Filter   
#define USE_AACBSF 0  

typedef enum{
	VFX_NULL = 0,
	VFX_EDGE = 1,
	VFX_NEGATE = 2
}VFX;

typedef enum{
	AFX_NULL = 0,
}AFX;

AVBitStreamFilterContext *aacbsfc = NULL;
AVBitStreamFilterContext* h264bsfc = NULL;
//multiple input
static AVFormatContext **ifmt_ctx;
AVFrame **frame = NULL;
//single output
static AVFormatContext *ofmt_ctx;

typedef struct FilteringContext {
	AVFilterContext *buffersink_ctx;
	AVFilterContext **buffersrc_ctx;
	AVFilterGraph *filter_graph;
} FilteringContext;
static FilteringContext *filter_ctx;

typedef struct InputFile{
	const char* filenames;
	/*
	* position index
	* 0 - 1 - 2
	* 3 - 4 - 5
	* 6 - 7 - 8
	* ……
	*/
	uint32_t video_idx;
	//scale level, 0 means keep the same
	//uint32_t video_expand;
	uint32_t video_effect;
	uint32_t audio_effect;
} InputFile;
InputFile* inputfiles;

typedef struct GlobalContext{
	//always be a square,such as 2x2, 3x3
	uint32_t grid_num;
	uint32_t video_num;
	uint32_t enc_width;
	uint32_t enc_height;
	uint32_t enc_bit_rate;
	InputFile* input_file;
	const char* outfilename;
} GlobalContext;
GlobalContext* global_ctx;

static int global_ctx_config()
{
	int i;
	if (global_ctx->grid_num < global_ctx->video_num)
	{
		av_log(NULL, AV_LOG_ERROR, "Setting a wrong grid_num %d \t The grid_num is smaller than video_num!! \n", global_ctx->grid_num);
		global_ctx->grid_num = global_ctx->video_num;
		//global_ctx->stride = sqrt((double)global_ctx->grid_num);
		av_log(NULL, AV_LOG_ERROR, "Automatically change the grid_num to be same as video_num!! \n");
	}

	//global_ctx->stride = sqrt((double)global_ctx->grid_num);
	
	for (i = 0; i < global_ctx->video_num; i++)
	{
		if (global_ctx->input_file[i].video_idx >= global_ctx->grid_num)
		{
			av_log(NULL, AV_LOG_ERROR, "Invalid video_inx value in the No.%d input\n", global_ctx->input_file[i].video_idx);
			return -1;
		}
	}
	return 0;
}


static int open_input_file(InputFile *input_file)
{
	int ret;
	unsigned int i;
	unsigned int j;

	ifmt_ctx = (AVFormatContext**)av_malloc((global_ctx->video_num)*sizeof(AVFormatContext*));
	for (i = 0; i < global_ctx->video_num; i++)
	{
		*(ifmt_ctx + i) = NULL;
		if ((ret = avformat_open_input((ifmt_ctx + i), input_file[i].filenames, NULL, NULL)) < 0) {
			av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
			return ret;
		}
		if ((ret = avformat_find_stream_info(ifmt_ctx[i], NULL)) < 0) {
			av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
			return ret;
		}
		for (j = 0; j < ifmt_ctx[i]->nb_streams; j++) {
			AVStream *stream;
			AVCodecContext *codec_ctx;
			stream = ifmt_ctx[i]->streams[j];
			codec_ctx = stream->codec;

			if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
				|| codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
				/* Open decoder */
				ret = avcodec_open2(codec_ctx,
					avcodec_find_decoder(codec_ctx->codec_id), NULL);
				if (ret < 0) {
					av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
					return ret;
				}
			}
		}
		av_dump_format(ifmt_ctx[i], 0, input_file[i].filenames, 0);
	}
	return 0;
}


static int open_output_file(const char *filename)
{
	AVStream *out_stream;
	AVStream *in_stream;
	AVCodecContext *dec_ctx, *enc_ctx;
	AVCodec *encoder;
	int ret;
	unsigned int i;
	ofmt_ctx = NULL;
	avformat_alloc_output_context2(&ofmt_ctx, NULL, "flv", filename);
	if (!ofmt_ctx) {
		av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
		return AVERROR_UNKNOWN;
	}
	for (i = 0; i < ifmt_ctx[0]->nb_streams; i++) {
		out_stream = avformat_new_stream(ofmt_ctx, NULL);
		if (!out_stream) {
			av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
			return AVERROR_UNKNOWN;
		}
		in_stream = ifmt_ctx[0]->streams[i];
		out_stream->time_base = in_stream->time_base;

		dec_ctx = in_stream->codec;
		enc_ctx = out_stream->codec;
		if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
			|| dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {			
			/* In this example, we transcode to same properties (picture size,
			* sample rate etc.). These properties can be changed for output
			* streams easily using filters */
			if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
				/* in this example, we choose transcoding to same codec */
				encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
				enc_ctx->height = global_ctx->enc_height;
				enc_ctx->width = global_ctx->enc_width;
				enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
				/* take first format from list of supported formats */
				enc_ctx->pix_fmt = encoder->pix_fmts[0];
				enc_ctx->me_range = 16;
				enc_ctx->max_qdiff = 4;
				enc_ctx->bit_rate = global_ctx->enc_bit_rate;
				enc_ctx->qcompress = 0.6;
				/* video time_base can be set to whatever is handy and supported by encoder */
				enc_ctx->time_base.num = 1;
				enc_ctx->time_base.den = 25;
				enc_ctx->gop_size = 250;
				enc_ctx->max_b_frames = 3;

				AVDictionary * d = NULL;
				char *k = av_strdup("preset");       // if your strings are already allocated,
				char *v = av_strdup("ultrafast");    // you can avoid copying them like this
				av_dict_set(&d, k, v, AV_DICT_DONT_STRDUP_KEY | AV_DICT_DONT_STRDUP_VAL);
				ret = avcodec_open2(enc_ctx, encoder, &d);
			}
			else {
				encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
				enc_ctx->sample_rate = dec_ctx->sample_rate;
				enc_ctx->channel_layout = dec_ctx->channel_layout;
				enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
				/* take first format from list of supported formats */
				enc_ctx->sample_fmt = encoder->sample_fmts[0];
				AVRational time_base = { 1, enc_ctx->sample_rate };
				enc_ctx->time_base = time_base;
				ret = avcodec_open2(enc_ctx, encoder, NULL);
			}
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder for stream #%u\n", i);
				return ret;
			}       
		}
		else if (dec_ctx->codec_type == AVMEDIA_TYPE_UNKNOWN) {
			av_log(NULL, AV_LOG_FATAL, "Elementary stream #%d is of unknown type, cannot proceed\n", i);
			return AVERROR_INVALIDDATA;
		}
		else {
			/* if this stream must be remuxed */
			ret = avcodec_copy_context(ofmt_ctx->streams[i]->codec,
				ifmt_ctx[0]->streams[i]->codec);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "Copying stream context failed\n");
				return ret;
			}
		}
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			enc_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}
	av_dump_format(ofmt_ctx, 0, filename, 1);
	if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", filename);
			return ret;
		}
	}
	/* init muxer, write output file header */
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
		return ret;
	}

#if USE_H264BSF  
	h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
#endif  
#if USE_AACBSF  
	aacbsfc = av_bitstream_filter_init("aac_adtstoasc");
#endif 

	return 0;
}


static int init_filter(FilteringContext* fctx, AVCodecContext **dec_ctx,
	AVCodecContext *enc_ctx, const char *filter_spec)
{
	char args[512];
	char pad_name[10];
	int ret = 0;
	int i;
	AVFilter **buffersrc = (AVFilter**)av_malloc(global_ctx->video_num*sizeof(AVFilter*));
	AVFilter *buffersink = NULL;
	AVFilterContext **buffersrc_ctx = (AVFilterContext**)av_malloc(global_ctx->video_num*sizeof(AVFilterContext*));
	AVFilterContext *buffersink_ctx = NULL;
	AVFilterInOut **outputs = (AVFilterInOut**)av_malloc(global_ctx->video_num*sizeof(AVFilterInOut*));
	AVFilterInOut *inputs = avfilter_inout_alloc();
	AVFilterGraph *filter_graph = avfilter_graph_alloc();

	for (i = 0; i < global_ctx->video_num; i++)
	{
		buffersrc[i] = NULL;
		buffersrc_ctx[i] = NULL;
		outputs[i] = avfilter_inout_alloc();
	}
	if (!outputs || !inputs || !filter_graph) {
		ret = AVERROR(ENOMEM);
		goto end;
	}
	if (dec_ctx[0]->codec_type == AVMEDIA_TYPE_VIDEO) {
		for (i = 0; i < global_ctx->video_num; i++)
		{
			buffersrc[i] = avfilter_get_by_name("buffer");
		}
		buffersink = avfilter_get_by_name("buffersink");
		if (!buffersrc || !buffersink) {
			av_log(NULL, AV_LOG_ERROR, "filtering source or sink element not found\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}

		for (i = 0; i < global_ctx->video_num; i++)
		{
			_snprintf(args, sizeof(args),
				"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
				dec_ctx[i]->width, dec_ctx[i]->height, dec_ctx[i]->pix_fmt,
				dec_ctx[i]->time_base.num, dec_ctx[i]->time_base.den,
				dec_ctx[i]->sample_aspect_ratio.num,
				dec_ctx[i]->sample_aspect_ratio.den);
			_snprintf(pad_name, sizeof(pad_name), "in%d", i);
			ret = avfilter_graph_create_filter(&(buffersrc_ctx[i]), buffersrc[i], pad_name,
				args, NULL, filter_graph);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
				goto end;
			}
		}

		ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
			NULL, NULL, filter_graph);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
			goto end;
		}
		ret = av_opt_set_bin(buffersink_ctx, "pix_fmts",
			(uint8_t*)&enc_ctx->pix_fmt, sizeof(enc_ctx->pix_fmt),
			AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
			goto end;
		}
	}
	else if (dec_ctx[0]->codec_type == AVMEDIA_TYPE_AUDIO) {
		for (i = 0; i < global_ctx->video_num; i++)
		{
			buffersrc[i] = avfilter_get_by_name("abuffer");
		}
		buffersink = avfilter_get_by_name("abuffersink");
		if (!buffersrc || !buffersink) {
			av_log(NULL, AV_LOG_ERROR, "filtering source or sink element not found\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}

		for (i = 0; i < global_ctx->video_num; i++)
		{
			if (!dec_ctx[i]->channel_layout)
				dec_ctx[i]->channel_layout =
				av_get_default_channel_layout(dec_ctx[i]->channels);
			_snprintf(args, sizeof(args),
				"time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%I64x",
				dec_ctx[i]->time_base.num, dec_ctx[i]->time_base.den, dec_ctx[i]->sample_rate,
				av_get_sample_fmt_name(dec_ctx[i]->sample_fmt),
				dec_ctx[i]->channel_layout);
			_snprintf(pad_name, sizeof(pad_name), "in%d", i);
			ret = avfilter_graph_create_filter(&(buffersrc_ctx[i]), buffersrc[i], pad_name,
				args, NULL, filter_graph);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
				goto end;
			}
		}
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
			goto end;
		}
		ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
			NULL, NULL, filter_graph);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
			goto end;
		}
		ret = av_opt_set_bin(buffersink_ctx, "sample_fmts",
			(uint8_t*)&enc_ctx->sample_fmt, sizeof(enc_ctx->sample_fmt),
			AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Cannot set output sample format\n");
			goto end;
		}
		ret = av_opt_set_bin(buffersink_ctx, "channel_layouts",
			(uint8_t*)&enc_ctx->channel_layout,
			sizeof(enc_ctx->channel_layout), AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Cannot set output channel layout\n");
			goto end;
		}
		ret = av_opt_set_bin(buffersink_ctx, "sample_rates",
			(uint8_t*)&enc_ctx->sample_rate, sizeof(enc_ctx->sample_rate),
			AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Cannot set output sample rate\n");
			goto end;
		}
	}
	/* Endpoints for the filter graph. */
	for (i = 0; i < global_ctx->video_num; i++)
	{
		_snprintf(pad_name, sizeof(pad_name), "in%d", i);
		outputs[i]->name = av_strdup(pad_name);
		outputs[i]->filter_ctx = buffersrc_ctx[i];
		outputs[i]->pad_idx = 0;
		if (i == global_ctx->video_num - 1)
			outputs[i]->next = NULL;
		else
			outputs[i]->next = outputs[i + 1];
	}

	inputs->name = av_strdup("out");
	inputs->filter_ctx = buffersink_ctx;
	inputs->pad_idx = 0;
	inputs->next = NULL;
	if (!outputs[0]->name || !inputs->name) {
		ret = AVERROR(ENOMEM);
		goto end;
	}
	if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_spec,
		&inputs, outputs, NULL)) < 0)
		goto end;
	if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
		goto end;
	/* Fill FilteringContext */
	fctx->buffersrc_ctx = buffersrc_ctx;
	fctx->buffersink_ctx = buffersink_ctx;
	fctx->filter_graph = filter_graph;
end:
	avfilter_inout_free(&inputs);
	av_free(buffersrc);
	//	av_free(buffersrc_ctx);
	avfilter_inout_free(outputs);
	av_free(outputs);

	return ret;
}


static int init_spec_filter(void)
{
	char filter_spec[512];
	char spec_temp[128];
	unsigned int i;
	unsigned int j;
	unsigned int k;
	unsigned int x_coor;
	unsigned int y_coor;
	AVCodecContext** dec_ctx_array;
	int	stream_num = ifmt_ctx[0]->nb_streams;
	int stride = (int)sqrt((long double)global_ctx->grid_num);
	int ret;
	filter_ctx = (FilteringContext *)av_malloc_array(stream_num, sizeof(*filter_ctx));
	dec_ctx_array = (AVCodecContext**)av_malloc(global_ctx->video_num*sizeof(AVCodecContext));

	if (!filter_ctx || !dec_ctx_array)
		return AVERROR(ENOMEM);
	for (i = 0; i < stream_num; i++) {
		filter_ctx[i].buffersrc_ctx = NULL;
		filter_ctx[i].buffersink_ctx = NULL;
		filter_ctx[i].filter_graph = NULL;
		for (j = 0; j < global_ctx->video_num; j++)
			dec_ctx_array[j] = ifmt_ctx[j]->streams[i]->codec;
		if (!(ifmt_ctx[0]->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO
			|| ifmt_ctx[0]->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO))
			continue;
		if (ifmt_ctx[0]->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			if (global_ctx->grid_num == 1)
				_snprintf(filter_spec, sizeof(filter_spec), "null");
			else
			{

				_snprintf(filter_spec, sizeof(filter_spec), "color=c=black@1:s=%dx%d[x0];", global_ctx->enc_width, global_ctx->enc_height);
				k = 1;
				for (j = 0; j < global_ctx->video_num; j++)
				{
					switch (global_ctx->input_file[j].video_effect)
					{
					case VFX_NULL:
						_snprintf(spec_temp, sizeof(spec_temp), "[in%d]null[ine%d];", j, j);
						strcat(filter_spec, spec_temp);
						break;
					case VFX_EDGE:
						_snprintf(spec_temp, sizeof(spec_temp), "[in%d]edgedetect[ine%d];", j, j);
						strcat(filter_spec, spec_temp);
						break;
					case VFX_NEGATE:
						_snprintf(spec_temp, sizeof(spec_temp), "[in%d]negate[ine%d];", j, j);
						strcat(filter_spec, spec_temp);
						break;
					}
						x_coor = global_ctx->input_file[j].video_idx % stride;
						y_coor = global_ctx->input_file[j].video_idx / stride;
						_snprintf(spec_temp, sizeof(spec_temp), "[ine%d]scale=w=%d:h=%d[inn%d];[x%d][inn%d]overlay=%d*%d/%d:%d*%d/%d[x%d];", j, global_ctx->enc_width / stride, global_ctx->enc_height / stride, j, k - 1, j, global_ctx->enc_width, x_coor, stride, global_ctx->enc_height, y_coor, stride, k);
						k++;
						strcat(filter_spec, spec_temp);
				}
				_snprintf(spec_temp, sizeof(spec_temp), "[x%d]null[out]", k - 1, global_ctx->enc_width, global_ctx->enc_height);
				strcat(filter_spec, spec_temp);
			}
		}
		else
		{
			if (global_ctx->video_num == 1)
				_snprintf(filter_spec, sizeof(filter_spec), "anull");
			else{
				_snprintf(filter_spec, sizeof(filter_spec), "");
				for (j = 0; j < global_ctx->video_num; j++)
				{
					_snprintf(spec_temp, sizeof(spec_temp), "[in%d]", j);
					strcat(filter_spec, spec_temp);
				}
				_snprintf(spec_temp, sizeof(spec_temp), "amix=inputs=%d[out]", global_ctx->video_num);
				strcat(filter_spec, spec_temp);
			}
		}

		ret = init_filter(&filter_ctx[i], dec_ctx_array,
			ofmt_ctx->streams[i]->codec, filter_spec);
		if (ret)
			return ret;
	}
	av_free(dec_ctx_array);
	return 0;
}

int videocombine(GlobalContext* video_ctx)
{
	int ret;
	int tmp = 0;
	int got_frame_num = 0;
	unsigned int stream_index;

	AVPacket packet;
	AVPacket enc_pkt;
	AVFrame* picref;
	enum AVMediaType mediatype;
	int read_frame_done = 0;
	int flush_now = 0;
	int framecnt = 0;
	int i, j;
	int got_frame;
	int enc_got_frame = 0;
	int(*dec_func)(AVCodecContext *, AVFrame *, int *, const AVPacket *);
	int(*enc_func)(AVCodecContext *, AVPacket *, const AVFrame *, int *);

	global_ctx = video_ctx;
	global_ctx_config();

	frame = (AVFrame**)av_malloc(global_ctx->video_num*sizeof(AVFrame*));
	picref = av_frame_alloc();
	av_register_all();
	avfilter_register_all();

	if ((ret = open_input_file(global_ctx->input_file)) < 0)
		goto end;
	if ((ret = open_output_file(global_ctx->outfilename)) < 0)
		goto end;
	if ((ret = init_spec_filter()) < 0)
		goto end;

	while (1) {
		for (i = 0; i < global_ctx->video_num; i++)
		{
			if (read_frame_done < 0)
			{
				flush_now = 1;
				goto flush;
			}			
			while ((read_frame_done=av_read_frame(ifmt_ctx[i], &packet)) >= 0)
			{
				stream_index = packet.stream_index;
				mediatype = ifmt_ctx[i]->streams[stream_index]->codec->codec_type;
				if (mediatype == AVMEDIA_TYPE_VIDEO || mediatype == AVMEDIA_TYPE_AUDIO)
				{
					frame[i] = av_frame_alloc();
					if (!(frame[i]))
					{
						ret = AVERROR(ENOMEM);
						goto end;
					}
					av_packet_rescale_ts(&packet,
						ifmt_ctx[i]->streams[stream_index]->time_base,
						ifmt_ctx[i]->streams[stream_index]->codec->time_base);
					dec_func = (mediatype == AVMEDIA_TYPE_VIDEO) ? avcodec_decode_video2 : avcodec_decode_audio4;
					ret = dec_func(ifmt_ctx[i]->streams[stream_index]->codec, frame[i], &got_frame, &packet);
					if (ret < 0)
					{
						av_frame_free(&frame[i]);
						av_log(NULL, AV_LOG_ERROR, "Decoding failed\n");
						goto end;
					}
					if (got_frame) {
						frame[i]->pts = av_frame_get_best_effort_timestamp(frame[i]);
						ret = av_buffersrc_add_frame(filter_ctx[stream_index].buffersrc_ctx[i], frame[i]);
						if (ret < 0) {
							av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
							goto end;
						}
					}
					else
					{
						av_frame_free(&(frame[i]));
					}
				}
				av_free_packet(&packet);
				if (got_frame)
				{
					got_frame = 0;
					break;
				}
			}
		}

		while (1) {
			ret = av_buffersink_get_frame_flags(filter_ctx[stream_index].buffersink_ctx, picref, 0);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			{
				ret = 0;
				break;
			}
			if (ret < 0)
				goto end;

			if (picref) {								
				enc_pkt.data = NULL;
				enc_pkt.size = 0;
				av_init_packet(&enc_pkt);
				enc_func = (mediatype == AVMEDIA_TYPE_VIDEO) ? avcodec_encode_video2 : avcodec_encode_audio2;
				ret = enc_func(ofmt_ctx->streams[stream_index]->codec, &enc_pkt,
					picref, &enc_got_frame);	
				if (ret < 0)
				{
					av_log(NULL, AV_LOG_ERROR, "Encoding failed\n");
					goto end;
				}
				if (enc_got_frame == 1){
					framecnt++;
					enc_pkt.stream_index = stream_index;

					//Write PTS
					AVRational time_base = ofmt_ctx->streams[stream_index]->time_base;//{ 1, 1000 };
					AVRational r_framerate1 = ifmt_ctx[0]->streams[stream_index]->r_frame_rate;// { 50, 2 };
					AVRational time_base_q = { 1, AV_TIME_BASE };
					//Duration between 2 frames (us)
					int64_t calc_duration = (double)(AV_TIME_BASE)*(1 / av_q2d(r_framerate1));	//内部时间戳
					//Parameters
					//enc_pkt.pts = (double)(framecnt*calc_duration)*(double)(av_q2d(time_base_q)) / (double)(av_q2d(time_base));
					enc_pkt.pts = av_rescale_q(framecnt*calc_duration, time_base_q, time_base);
					enc_pkt.dts = enc_pkt.pts;
					enc_pkt.duration = av_rescale_q(calc_duration, time_base_q, time_base); //(double)(calc_duration)*(double)(av_q2d(time_base_q)) / (double)(av_q2d(time_base));
					enc_pkt.pos = -1;

#if USE_H264BSF  
					av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif  
#if USE_AACBSF  
					av_bitstream_filter_filter(aacbsfc, out_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif  

					ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
					av_log(NULL, AV_LOG_INFO, "write frame %d\n", framecnt);
					av_free_packet(&enc_pkt);
				}
				av_frame_unref(picref);
			}	
		}
	}

flush:
	/* flush filters and encoders */
	for (i = 0; i < ifmt_ctx[0]->nb_streams; i++) {
		stream_index = i;

		/* flush encoder */
		if (!(ofmt_ctx->streams[stream_index]->codec->codec->capabilities &
			CODEC_CAP_DELAY))
			return 0;
		while (1) {
			enc_pkt.data = NULL;
			enc_pkt.size = 0;
			av_init_packet(&enc_pkt);
			enc_func = (ifmt_ctx[0]->streams[stream_index]->codec->codec_type == AVMEDIA_TYPE_VIDEO) ?
			avcodec_encode_video2 : avcodec_encode_audio2;
			ret = enc_func(ofmt_ctx->streams[stream_index]->codec, &enc_pkt,
				NULL, &enc_got_frame);
			av_frame_free(NULL);
			if (ret < 0)
			{
				av_log(NULL, AV_LOG_ERROR, "Encoding failed\n");
				goto end;
			}
			if (!enc_got_frame){
				ret = 0;
				break;
			}
			printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);

			//Write PTS
			AVRational time_base = ofmt_ctx->streams[stream_index]->time_base;//{ 1, 1000 };
			AVRational r_framerate1 = ifmt_ctx[0]->streams[stream_index]->r_frame_rate;// { 50, 2 };
			AVRational time_base_q = { 1, AV_TIME_BASE };
			//Duration between 2 frames (us)
			int64_t calc_duration = (double)(AV_TIME_BASE)*(1 / av_q2d(r_framerate1));	//内部时间戳
			//Parameters
			enc_pkt.pts = av_rescale_q(framecnt*calc_duration, time_base_q, time_base);
			enc_pkt.dts = enc_pkt.pts;
			enc_pkt.duration = av_rescale_q(calc_duration, time_base_q, time_base);

			/* copy packet*/
			//转换PTS/DTS（Convert PTS/DTS）
			enc_pkt.pos = -1;
			framecnt++;
			ofmt_ctx->duration = enc_pkt.duration * framecnt;

			/* mux encoded frame */
			ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
		}
	}

	av_write_trailer(ofmt_ctx);

#if USE_H264BSF  
	av_bitstream_filter_close(h264bsfc);
#endif  
#if USE_AACBSF  
	av_bitstream_filter_close(aacbsfc);
#endif  

end:
	av_free_packet(&packet);
	for (i = 0; i < global_ctx->video_num; i++)
	{
		av_frame_free(&(frame[i]));
		for (j = 0; j < ofmt_ctx->nb_streams; j++) {
			avcodec_close(ifmt_ctx[i]->streams[j]->codec);
		}
	}
	av_free(frame);
	av_free(picref);
	for (i = 0; i<ofmt_ctx->nb_streams; i++)
	{
		if (ofmt_ctx && ofmt_ctx->nb_streams > i && ofmt_ctx->streams[i] && ofmt_ctx->streams[i]->codec)
			avcodec_close(ofmt_ctx->streams[i]->codec);
		av_free(filter_ctx[i].buffersrc_ctx);
		if (filter_ctx && filter_ctx[i].filter_graph)
			avfilter_graph_free(&filter_ctx[i].filter_graph);
	}

	av_free(filter_ctx);
	for (i = 0; i < global_ctx->video_num; i++)
		avformat_close_input(&(ifmt_ctx[i]));
	if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
	av_free(ifmt_ctx);

	if (ret < 0)
		av_log(NULL, AV_LOG_ERROR, "Error occurred\n");
	return (ret ? 1 : 0);
}


int main(int argc, char **argv)
{
	//test 2x2
	inputfiles = (InputFile*)av_malloc_array(4, sizeof(InputFile));
	if (!inputfiles)
		return AVERROR(ENOMEM);

	
	inputfiles[0].filenames = "in1.flv";
	//inputfiles[0].video_expand = 0;
	inputfiles[0].video_idx = 0;
	inputfiles[0].video_effect = VFX_EDGE;
	inputfiles[0].audio_effect = AFX_NULL;
	inputfiles[1].filenames = "in2.flv";
	//inputfiles[1].video_expand = 0;
	inputfiles[1].video_idx = 1;
	inputfiles[1].video_effect = VFX_NULL;
	inputfiles[1].audio_effect = AFX_NULL;
	inputfiles[2].filenames = "in3.flv";
	//inputfiles[2].video_expand = 0;
	inputfiles[2].video_idx = 2;
	inputfiles[2].video_effect = VFX_NULL;
	inputfiles[2].audio_effect = AFX_NULL;
	inputfiles[3].filenames = "in4.flv";
	//inputfiles[3].video_expand = 0;
	inputfiles[3].video_idx = 3;
	inputfiles[3].video_effect = VFX_NEGATE;
	inputfiles[3].audio_effect = AFX_NULL;

	global_ctx = (GlobalContext*)av_malloc(sizeof(GlobalContext));
	global_ctx->video_num = 4;
	global_ctx->grid_num = 4;
	global_ctx->enc_bit_rate = 500000;
	global_ctx->enc_height = 360;
	global_ctx->enc_width = 640;
	global_ctx->outfilename = "combined.flv";
	global_ctx->input_file = inputfiles;

	return videocombine(global_ctx);
}