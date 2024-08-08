#include <string>
#include <unordered_map>
#include <arpa/inet.h>
#include <iostream>

#include "Common/Track.h"
#include "Common/Frame.h"
#include "Net/Buffer.h"
#include "TsMuxer.h"
#include "Log/Logger.h"
#include "Mpeg.h"
#include "Util/CRC32.h"

/* 
 *remark: 上面用到的一些宏定义和一些关于字节操作的函数，很多一些开源到视频处理的库都能看到，
          为了方便也都将贴出来分享，当然也可以参考下vlc里面的源码
 */
 
/*@remark: 常量定义 */
#define TS_PID_PMT		(0x62)
#define TS_PID_VIDEO	(0x65)
#define TS_PID_AUDIO	(0x84)
#define TS_PMT_STREAMTYPE_11172_AUDIO	(0x03)
#define TS_PMT_STREAMTYPE_13818_AUDIO	(0x04)
#define TS_PMT_STREAMTYPE_AAC_AUDIO		(0x0F)
#define TS_PMT_STREAMTYPE_H264_VIDEO	(0x1B)

#define TS_LOAD_LEN 188

#define TS_PES_PAYLOAD_SIZE 65522

enum {
    TS_TYPE_PAT,
    TS_TYPE_PMT,
    TS_TYPE_VIDEO,
    TS_TYPE_AUDIO
};
 
typedef struct
{
	unsigned int startcode		: 24;	// 固定为00 00 01
	unsigned int stream_id		: 8;	// 0xC0-0xDF audio stream, 0xE0-0xEF video stream, 0xBD Private stream 1, 0xBE Padding stream, 0xBF Private stream 2
	unsigned short pack_len;			// PES packet length
} __attribute__ ((packed)) PES_HEAD_S;
 
typedef struct
{
#if (BYTE_ORDER == LITTLE_ENDIAN)
	unsigned char original		: 1;	// original or copy, 原版或拷贝
	unsigned char copyright		: 1;	// copyright flag
	unsigned char align			: 1;	// data alignment indicator, 数据定位指示符
	unsigned char priority		: 1;	// PES priority
	unsigned char scramb		: 2;	// PES Scrambling control, 加扰控制
	unsigned char fixed			: 2;	// 固定为10
 
	unsigned char exten			: 1;	// PES extension flag
	unsigned char crc			: 1;	// PES CRC flag
	unsigned char acopy			: 1;	// additional copy info flag
	unsigned char trick			: 1;	// DSM(Digital Storage Media) trick mode flag
	unsigned char rate			: 1;	// ES rate flag, ES流速率标志
	unsigned char escr			: 1;	// ESCR(Elementary Stream Clock Reference) flag, ES流时钟基准标志
	unsigned char pts_dts		: 2;	// PTS DTS flags, 00 no PTS and DTS, 01 forbid, 10 have PTS, 11 have PTS and DTS
#elif (BYTE_ORDER == BIG_ENDIAN)
	unsigned char fixed			: 2;	// 固定为10
	unsigned char scramb		: 2;	// PES Scrambling control, 加扰控制
	unsigned char priority		: 1;	// PES priority
	unsigned char align			: 1;	// data alignment indicator, 数据定位指示符
	unsigned char copyright		: 1;	// copyright flag
	unsigned char original		: 1;	// original or copy, 原版或拷贝
	
	unsigned char pts_dts		: 2;	// PTS DTS flags, 00 no PTS and DTS, 01 forbid, 10 have PTS, 11 have PTS and DTS
	unsigned char escr			: 1;	// ESCR(Elementary Stream Clock Reference) flag, ES流时钟基准标志
	unsigned char rate			: 1;	// ES rate flag, ES流速率标志
	unsigned char trick			: 1;	// DSM(Digital Storage Media) trick mode flag
	unsigned char acopy			: 1;	// additional copy info flag
	unsigned char crc			: 1;	// PES CRC flag
	unsigned char exten			: 1;	// PES extension flag
#endif
 
	unsigned char head_len;				// PES header data length
} __attribute__ ((packed)) PES_OPTION_S;
 
typedef struct
{// ts total 33 bits
#if (BYTE_ORDER == LITTLE_ENDIAN)
	unsigned char fixed2		: 1;	// 固定为1
	unsigned char ts1			: 3;	// bit30-32
	unsigned char fixed1		: 4;	// DTS为0x01, PTS为0x02, PTS+DTS则PTS为0x03
	
	unsigned char ts2;					// bit22-29
	unsigned char fixed3		: 1;	// 固定为1
	unsigned char ts3			: 7;	// bit15-21
 
	unsigned char ts4;					// bit7-14
	unsigned char fixed4		: 1;	// 固定为1
	unsigned char ts5			: 7;	// bit0-6
#elif (BYTE_ORDER == BIG_ENDIAN)
	unsigned char fixed1		: 4;	// DTS为0x01, PTS为0x02, PTS+DTS则PTS为0x03
	unsigned char ts1			: 3;	// bit30-32
	unsigned char fixed2		: 1;	// 固定为1
 
	unsigned char ts2;					// bit22-29
	unsigned char ts3			: 7;	// bit15-21
	unsigned char fixed3		: 1;	// 固定为1
 
	unsigned char ts4;					// bit7-14
	unsigned char ts5			: 7;	// bit0-6
	unsigned char fixed4		: 1;	// 固定为1
#endif
} __attribute__ ((packed)) PES_PTS_S;
 
 
/* remark:接口函数定义 */
int bits_initwrite(bits_buffer_s *p_buffer, int i_size, unsigned char *p_data)
{
	if (!p_data)
	{
		return -1;
	}
	p_buffer->i_size = i_size;
	p_buffer->i_data = 0;
	p_buffer->i_mask = 0x80;
	p_buffer->p_data = p_data;
	p_buffer->p_data[0] = 0;
	return 0;
}
 
void bits_align(bits_buffer_s *p_buffer)
{
	if (p_buffer->i_mask != 0x80 && p_buffer->i_data < p_buffer->i_size)
	{
		p_buffer->i_mask = 0x80;
		p_buffer->i_data++;
		p_buffer->p_data[p_buffer->i_data] = 0x00;
	}
}

TsMuxer::TsMuxer()
{
    logInfo << "TsMuxer";
}

TsMuxer::~TsMuxer()
{
    logInfo << "~TsMuxer";
}

void TsMuxer::onFrame(const FrameBuffer::Ptr& frame)
{
    if (!_startEncode) {
        return ;
    }
    encode(frame);
}

void TsMuxer::startEncode()
{
    _startEncode = true;
	_mapContinuity[TS_PID_AUDIO] = 0;
	_mapContinuity[TS_PID_VIDEO] = 0;
	_mapContinuity[TS_PID_PMT] = 0;
}

void TsMuxer::stopEncode()
{
    _startEncode = false;
}

void TsMuxer::addTrackInfo(const shared_ptr<TrackInfo>& trackInfo)
{
    _mapTrackInfo[trackInfo->index_] = trackInfo;
    if (trackInfo->trackType_ == "video") {
        _mapStreamId[trackInfo->index_] = _lastVideoId++;
        if (trackInfo->codec_ == "h264") {
            _videoCodec = STREAM_TYPE_VIDEO_H264;
        } else if (trackInfo->codec_ == "h265") {
            _videoCodec = STREAM_TYPE_VIDEO_HEVC;
        }
    } else if (trackInfo->trackType_ == "audio") {
        _mapStreamId[trackInfo->index_] = _lastAudioId++;
        if (trackInfo->codec_ == "aac") {
            _audioCodec = STREAM_TYPE_AUDIO_AAC;
        } else if (trackInfo->codec_ == "g711a") {
            _audioCodec = STREAM_TYPE_AUDIO_G711;
        } else if (trackInfo->codec_ == "g711u") {
            _audioCodec = STREAM_TYPE_AUDIO_G711ULAW;
        }
    }
}

void TsMuxer::onTsPacket(const StreamBuffer::Ptr& frame, int pts, int dts, bool keyframe)
{
	if (_onTsPacket) {
		_onTsPacket(frame, pts, dts, keyframe);
	}
}
 
/***
 *@remark:  音视频数据的打包成ps流，并封装成rtp
 *@param :  pData      [in] 需要发送的音视频数据
 *          nFrameLen  [in] 发送数据的长度
 *          pPacker    [in] 数据包的一些信息，包括时间戳，rtp数据buff，发送的socket相关信息
 *          stream_type[in] 数据类型 0 视频 1 音频
 *@return:  0 success others failed
*/
 
int TsMuxer::encode(const FrameBuffer::Ptr& frame)
{
    auto tsPacket = make_shared<StreamBuffer>();
    tsPacket->setCapacity(188 + 1);

    int nRet = 0;
	int bVideo = frame->getTrackType() == VideoTrackType;
	int nSendDataOff = 0;
	// int nSendSize    = 0;	
	// int nPayLoadSize = 0;
	// int nHasSend     = 0;
	// int IFrameFlag   = 0;
	// char TSFrameHdr[1024];
	// int nHead = 0;

	// logInfo << "frame type: " << (int)frame->getNalType();

    if (_first || (frame->getTrackType() == VideoTrackType && (frame->keyFrame() || frame->metaFrame()))) {
		_first = false;
		
        if((nRet = mk_ts_pat_packet(tsPacket->data() +nSendDataOff, 
						0)) <= 0)	
		{
            logInfo << "mk_ts_pat_packet failed!";
			return -1;
		}
		onTsPacket(tsPacket, frame->pts(), frame->dts(), frame->keyFrame() || frame->metaFrame());
		// @remark: 每次添加一个头的时候，必须注意指针到偏移量
		// nSendDataOff += nRet;
		tsPacket = make_shared<StreamBuffer>();
    	tsPacket->setCapacity(188 + 1);
		if((nRet = mk_ts_pmt_packet(tsPacket->data() + nSendDataOff, 
						0)) <= 0)	
		{
            logInfo << "mk_ts_pmt_packet failed!";
			return -1;
		}
		// nSendDataOff += nRet;
		onTsPacket(tsPacket, frame->pts(), frame->dts(), frame->keyFrame() || frame->metaFrame());
    }

	make_pes_packet(frame);

	return 0;
}

int TsMuxer::make_pes_packet(const FrameBuffer::Ptr& frame)
{
	bits_buffer_s bits;
	auto tsPacket = make_shared<StreamBuffer>();
    tsPacket->setCapacity(TS_LOAD_LEN + 1);

	// bits.i_size = 32; 
    // bits.i_data = 0;
    // bits.i_mask = 0x80; // 二进制：10000000 这里是为了后面对一个字节的每一位进行操作，避免大小端夸字节字序错乱
    // bits.p_data = (unsigned char *)(tsPacket->data());

	// memset(bits.p_data, 0xff, TS_LOAD_LEN);

	// pcr
	// bits_write(&bits, 8, 0x47); //ts包起始字节
	// bits_write(&bits, 1, 0);			// transport error indicator
	// bits_write(&bits, 1, 0);		// payload unit start indicator
	// bits_write(&bits, 1, 0);			// transport priority, 1表示高优先级
	// bits_write(&bits, 13, TS_PID_VIDEO);	// pid, 0x00 PAT, 0x01 CAT

	// bits_write(&bits, 2, 0);			// transport scrambling control, 传输加扰控制
	// bits_write(&bits, 2, 0x02);		// 第一位表示有无调整字段，第二位表示有无有效负载
	// bits_write(&bits, 4, 0);

	// bits_write(&bits, 8, 0xb7); //length
	// bits_write(&bits, 8, 0x10); // pcr flag
	// bits_write(&bits, 33, frame->dts()); // pcr
	// bits_write(&bits, 6, 0x3f); //reserved
	// bits_write(&bits, 9, 0); //program clock reference extension

	// onTsPacket(tsPacket, frame->pts(), frame->dts(), frame->keyFrame() || frame->metaFrame());

	// pes start

	// tsPacket = make_shared<StreamBuffer>();
    // tsPacket->setCapacity(TS_LOAD_LEN + 1);

    int nRet = 0;
	int bVideo = frame->getTrackType() == VideoTrackType;
	int nSendDataOff = 0;

	// unsigned char packet_pes = TS_LOAD_LEN - 4;    //每个ts包除包头后的长度
	// unsigned char packet_remain = frame->size() % packet_pes;    //每个ts包长度规定188字节，再去掉ts头长度
	// unsigned char packet_num = frame->size() / packet_pes;    //可以剩余可封装出的不含自适应区的包数



	int pid = bVideo ? TS_PID_VIDEO : TS_PID_AUDIO;
	int frameSize = frame->size();
	int pesSize = frame->size() + 14; //frame size + pes header size

	if (frame->_codec == "h265") {
		pesSize += 7;
	} else if (frame->_codec == "h264") {
		pesSize += 6;
	}
    
    bits.i_size = 32; 
    bits.i_data = 0;
    bits.i_mask = 0x80; // 二进制：10000000 这里是为了后面对一个字节的每一位进行操作，避免大小端夸字节字序错乱
    bits.p_data = (unsigned char *)(tsPacket->data());

	if (pesSize < TS_LOAD_LEN - 4) {
		bits_write(&bits, 8, 0x47); //ts包起始字节
		bits_write(&bits, 1, 0);			// transport error indicator
		bits_write(&bits, 1, 1);		// payload unit start indicator
		bits_write(&bits, 1, 0);			// transport priority, 1表示高优先级
		bits_write(&bits, 13, pid);	// pid, 0x00 PAT, 0x01 CAT

		bits_write(&bits, 2, 0);			// transport scrambling control, 传输加扰控制
		bits_write(&bits, 2, 0x03);		// 第一位表示有无调整字段，第二位表示有无有效负载
		bits_write(&bits, 4, _mapContinuity[pid]); // continuity counter, 0~15
		_mapContinuity[pid]++;
		_mapContinuity[pid] &= 0x0F;

		unsigned char stuff_num = TS_LOAD_LEN - 4 - 1 - pesSize;    //填充字节数 = ts包长（188字节） - ts头（4字节） - 自适应域长度（1字节） - 有效载荷长度
		bits_write(&bits, 8, stuff_num); // continuity counter, 0~15

		if (stuff_num > 0) {
			bits_write(&bits, 8, 0x00); //8bit填充信息flag,我们不要额外的设定信息，填充字节全部0xff
			memset(bits.p_data + bits.i_data, 0xff, stuff_num - 1);

			bits.i_data += stuff_num - 1;
		}

		if((nRet = mk_pes_packet(tsPacket->data() + bits.i_data, bVideo, pesSize - 14, 1, 
						frame->pts(), frame->dts())) <= 0 )
		{
			logInfo << "mk_pes_packet failed!";
			return -1;
		}
		bits.i_data += nRet;

		if (frame->_codec == "h265") {
			bits_write(&bits, 32, 0x00000001);
			bits_write(&bits, 8, 0x46);
			bits_write(&bits, 8, 0x01);
			bits_write(&bits, 8, 0x50);
		} else if (frame->_codec == "h264") {
			bits_write(&bits, 32, 0x00000001);
			bits_write(&bits, 8, 0x09);
			bits_write(&bits, 8, 0xF0);
		}

		memcpy(bits.p_data + bits.i_data, frame->data() + nSendDataOff, TS_LOAD_LEN - bits.i_data);
		nSendDataOff += TS_LOAD_LEN - bits.i_data;

		onTsPacket(tsPacket, frame->pts(), frame->dts(), frame->keyFrame() || frame->metaFrame());

		return 0;
	} else {
		bits_write(&bits, 8, 0x47); //ts包起始字节
		bits_write(&bits, 1, 0);			// transport error indicator
		bits_write(&bits, 1, 1);		// payload unit start indicator
		bits_write(&bits, 1, 0);			// transport priority, 1表示高优先级
		bits_write(&bits, 13, pid);	// pid, 0x00 PAT, 0x01 CAT

		bits_write(&bits, 2, 0);			// transport scrambling control, 传输加扰控制

		// continuity counter, 是具有同一PID值的TS包之间的连续计数值
		// 当分组的adaption_field_control字段为00话10时，该字段不递增
		bits_write(&bits, 2, 0x03);		// 第一位表示有无调整字段，第二位表示有无有效负载
		bits_write(&bits, 4, _mapContinuity[pid]); // continuity counter, 0~15
		_mapContinuity[pid]++;
		_mapContinuity[pid] &= 0x0F;

		bits_write(&bits, 8, 0x00);

		if((nRet = mk_pes_packet(tsPacket->data() + bits.i_data, bVideo, pesSize - 14, 1, 
						frame->pts(), frame->dts())) <= 0 )
		{
			logInfo << "mk_pes_packet failed!";
			return -1;
		}
		bits.i_data += nRet;

		if (frame->_codec == "h265") {
			bits_write(&bits, 32, 0x00000001);
			bits_write(&bits, 8, 0x46);
			bits_write(&bits, 8, 0x01);
			bits_write(&bits, 8, 0x50);
		} else if (frame->_codec == "h264") {
			bits_write(&bits, 32, 0x00000001);
			bits_write(&bits, 8, 0x09);
			bits_write(&bits, 8, 0xF0);
		}

		memcpy(bits.p_data + bits.i_data, frame->data() + nSendDataOff, TS_LOAD_LEN - bits.i_data);
		nSendDataOff += TS_LOAD_LEN - bits.i_data;

		onTsPacket(tsPacket, frame->pts(), frame->dts(), frame->keyFrame() || frame->metaFrame());
	}
	

	while (frameSize > nSendDataOff) {
		tsPacket = make_shared<StreamBuffer>();
		tsPacket->setCapacity(TS_LOAD_LEN + 1);

		bits.i_size = 32; 
		bits.i_data = 0;
		bits.i_mask = 0x80;
		bits.p_data = (unsigned char *)(tsPacket->data());

		if (frameSize - nSendDataOff >= TS_LOAD_LEN - 4) {
			bits_write(&bits, 8, 0x47); //ts包起始字节
			bits_write(&bits, 1, 0);			// transport error indicator
			bits_write(&bits, 1, 0);		// payload unit start indicator
			bits_write(&bits, 1, 0);			// transport priority, 1表示高优先级
			bits_write(&bits, 13, pid);	// pid, 0x00 PAT, 0x01 CAT

			bits_write(&bits, 2, 0);			// transport scrambling control, 传输加扰控制

			// continuity counter, 是具有同一PID值的TS包之间的连续计数值
			// 当分组的adaption_field_control字段为00话10时，该字段不递增
			bits_write(&bits, 2, 0x01);		// 第一位表示有无调整字段，第二位表示有无有效负载
			bits_write(&bits, 4, _mapContinuity[pid]); // continuity counter, 0~15
			_mapContinuity[pid]++;
			_mapContinuity[pid] &= 0x0F;

			memcpy(bits.p_data + bits.i_data, frame->data() + nSendDataOff, TS_LOAD_LEN - bits.i_data);
			nSendDataOff += TS_LOAD_LEN - bits.i_data;

			onTsPacket(tsPacket, frame->pts(), frame->dts(), frame->keyFrame() || frame->metaFrame());
		} else {
			bits_write(&bits, 8, 0x47); //ts包起始字节
			bits_write(&bits, 1, 0);			// transport error indicator
			bits_write(&bits, 1, 0);		// payload unit start indicator
			bits_write(&bits, 1, 0);			// transport priority, 1表示高优先级
			bits_write(&bits, 13, pid);	// pid, 0x00 PAT, 0x01 CAT

			bits_write(&bits, 2, 0);			// transport scrambling control, 传输加扰控制

			// continuity counter, 是具有同一PID值的TS包之间的连续计数值
			// 当分组的adaption_field_control字段为00话10时，该字段不递增
			bits_write(&bits, 2, 0x03);		// 第一位表示有无调整字段，第二位表示有无有效负载
			bits_write(&bits, 4, _mapContinuity[pid]); // continuity counter, 0~15
			_mapContinuity[pid]++;
			_mapContinuity[pid] &= 0x0F;

			unsigned char stuff_num = TS_LOAD_LEN - 4 - 1 - (frameSize - nSendDataOff);    //填充字节数 = ts包长（188字节） - ts头（4字节） - 自适应域长度（1字节） - 有效载荷长度
			bits_write(&bits, 8, stuff_num); // continuity counter, 0~15

			if (stuff_num > 0) {
				bits_write(&bits, 8, 0x00); //8bit填充信息flag,我们不要额外的设定信息，填充字节全部0xff
				memset(bits.p_data + bits.i_data, 0xff, stuff_num - 1);

				bits.i_data += stuff_num - 1;
			}

			memcpy(bits.p_data + bits.i_data, frame->data() + nSendDataOff, TS_LOAD_LEN - bits.i_data);
			nSendDataOff += TS_LOAD_LEN - bits.i_data;

			onTsPacket(tsPacket, frame->pts(), frame->dts(), frame->keyFrame() || frame->metaFrame());
		}
	}

	return 0;
}

// int TsMuxer::make_pes_packet(const FrameBuffer::Ptr& frame)
// {
// 	auto tsPacket = make_shared<StreamBuffer>();
//     tsPacket->setCapacity(TS_LOAD_LEN + 1);

//     int nRet = 0;
// 	int bVideo = frame->getTrackType() == VideoTrackType;
// 	int nSendDataOff = 0;

// 	unsigned char packet_pes = TS_LOAD_LEN - 4;    //每个ts包除包头后的长度
// 	unsigned char packet_remain = frame->size() % packet_pes;    //每个ts包长度规定188字节，再去掉ts头长度
// 	unsigned char packet_num = frame->size() / packet_pes;    //可以剩余可封装出的不含自适应区的包数


// 	bits_buffer_s bits;

// 	int pid = bVideo ? TS_PID_VIDEO : TS_PID_AUDIO;
    
//     bits.i_size = 32; 
//     bits.i_data = 0;
//     bits.i_mask = 0x80; // 二进制：10000000 这里是为了后面对一个字节的每一位进行操作，避免大小端夸字节字序错乱
//     bits.p_data = (unsigned char *)(tsPacket->data());

// 	bits_write(&bits, 8, 0x47); //ts包起始字节
// 	bits_write(&bits, 1, 0);			// transport error indicator
// 	bits_write(&bits, 1, 1);		// payload unit start indicator
// 	bits_write(&bits, 1, 1);			// transport priority, 1表示高优先级
// 	bits_write(&bits, 13, pid);	// pid, 0x00 PAT, 0x01 CAT

// 	if (packet_remain  > 0) {
// 		bits_write(&bits, 2, 0);			// transport scrambling control, 传输加扰控制
// 		bits_write(&bits, 2, 0x03);		// 第一位表示有无调整字段，第二位表示有无有效负载
// 		bits_write(&bits, 4, _mapContinuity[pid]); // continuity counter, 0~15
// 		_mapContinuity[pid]++;
// 		_mapContinuity[pid] &= 0x0F;

// 		unsigned char stuff_num = TS_LOAD_LEN - 4 - 1 - packet_remain;    //填充字节数 = ts包长（188字节） - ts头（4字节） - 自适应域长度（1字节） - 有效载荷长度
// 		bits_write(&bits, 8, stuff_num); // continuity counter, 0~15

// 		if (stuff_num > 0) {
// 			bits_write(&bits, 8, 0x00); //8bit填充信息flag,我们不要额外的设定信息，填充字节全部0xff
// 			memset(bits.p_data + bits.i_data, 0xff, stuff_num - 1);

// 			bits.i_data += stuff_num - 1;
// 		}

// 		int nFrameLen = frame->size();
// 		if((nRet = mk_pes_packet(tsPacket->data() + bits.i_data, bVideo, nFrameLen, 1, 
// 						frame->pts(), frame->dts())) <= 0 )
// 		{
// 			logInfo << "mk_pes_packet failed!";
// 			return -1;
// 		}
// 		bits.i_data += nRet;

// 		memcpy(bits.p_data + bits.i_data, frame->data() + nSendDataOff, TS_LOAD_LEN - bits.i_data);
// 		nSendDataOff += TS_LOAD_LEN - bits.i_data;
// 	} else {
// 		bits_write(&bits, 2, 0);			// transport scrambling control, 传输加扰控制

// 		// continuity counter, 是具有同一PID值的TS包之间的连续计数值
// 		// 当分组的adaption_field_control字段为00话10时，该字段不递增
// 		bits_write(&bits, 2, 0x01);		// 第一位表示有无调整字段，第二位表示有无有效负载
// 		bits_write(&bits, 4, _mapContinuity[pid]); // continuity counter, 0~15
// 		_mapContinuity[pid]++;
// 		_mapContinuity[pid] &= 0x0F;

// 		int nFrameLen = frame->size();
// 		if((nRet = mk_pes_packet(tsPacket->data() + bits.i_data, bVideo, nFrameLen, 1, 
// 						frame->pts(), frame->dts())) <= 0 )
// 		{
// 			logInfo << "mk_pes_packet failed!";
// 			return -1;
// 		}
// 		bits.i_data += nRet;

// 		memcpy(bits.p_data + bits.i_data, frame->data() + nSendDataOff, TS_LOAD_LEN - bits.i_data);
// 		nSendDataOff += TS_LOAD_LEN - bits.i_data;
// 	}
// 	onTsPacket(tsPacket);

// 	while (packet_num > 0) {
// 		tsPacket = make_shared<StreamBuffer>();
// 		tsPacket->setCapacity(TS_LOAD_LEN + 1);

// 		bits.i_size = 32; 
// 		bits.i_data = 0;
// 		bits.i_mask = 0x80;
// 		bits.p_data = (unsigned char *)(tsPacket->data());

// 		bits_write(&bits, 8, 0x47); //ts包起始字节
// 		bits_write(&bits, 1, 0);			// transport error indicator
// 		bits_write(&bits, 1, 1);		// payload unit start indicator
// 		bits_write(&bits, 1, 1);			// transport priority, 1表示高优先级
// 		bits_write(&bits, 13, pid);	// pid, 0x00 PAT, 0x01 CAT

// 		bits_write(&bits, 2, 0);			// transport scrambling control, 传输加扰控制

// 		// continuity counter, 是具有同一PID值的TS包之间的连续计数值
// 		// 当分组的adaption_field_control字段为00话10时，该字段不递增
// 		bits_write(&bits, 2, 0x01);		// 第一位表示有无调整字段，第二位表示有无有效负载
// 		bits_write(&bits, 4, _mapContinuity[pid]); // continuity counter, 0~15
// 		_mapContinuity[pid]++;
// 		_mapContinuity[pid] &= 0x0F;

// 		memcpy(bits.p_data + bits.i_data, frame->data() + nSendDataOff, TS_LOAD_LEN - bits.i_data);
// 		nSendDataOff += TS_LOAD_LEN - bits.i_data;
// 		--packet_num;
// 		onTsPacket(tsPacket);
// 	}

// 	return 0;
// }

/* 
 *@remark : 添加pat头 
 */
int TsMuxer::mk_ts_pat_packet(char *buf, int handle)
{
	int nOffset = 0;
	int nRet = 0;
	
	bits_buffer_s bits;
	
	if (!buf)
	{
		return 0;
	}
    
    bits.i_size = 32; 
    bits.i_data = 0;
    bits.i_mask = 0x80; // 二进制：10000000 这里是为了后面对一个字节的每一位进行操作，避免大小端夸字节字序错乱
    bits.p_data = (unsigned char *)(buf);

	bits_write(&bits, 8, 0x47); //ts包起始字节
	bits_write(&bits, 1, 0);			// transport error indicator
	bits_write(&bits, 1, 1);		// payload unit start indicator
	bits_write(&bits, 1, 1);			// transport priority, 1表示高优先级
	bits_write(&bits, 13, 0x00);	// pid, 0x00 PAT, 0x01 CAT
 
	bits_write(&bits, 2, 0);			// transport scrambling control, 传输加扰控制

	// continuity counter, 是具有同一PID值的TS包之间的连续计数值
	// 当分组的adaption_field_control字段为00话10时，该字段不递增
	bits_write(&bits, 2, 0x01);		// adaptation field control, 00 forbid, 01 have payload, 10 have adaptation, 11 have payload and adaptation
	bits_write(&bits, 4, _mapContinuity[0]); // continuity counter, 0~15
	_mapContinuity[0]++;
	_mapContinuity[0] &= 0x0F;

	bits_write(&bits, 8, 0x00); //因为前面负载开始标识为1，这里有一个调整字节,一般这个字节为0x00

	// start pat
	bits_write(&bits, 8, TS_TYPE_PAT); //table_id,对于PAT只能是0x00

	bits_write(&bits, 1, 1);				// section syntax indicator, 固定为1
	bits_write(&bits, 1, 0);				// zero, 0
	bits_write(&bits, 2, 0x03);				// reserved1, 固定为0x03
	bits_write(&bits, 12, 0x0D);	 		// section length, 表示这个字节后面有用的字节数, 包括CRC32
	
	bits_write(&bits, 16, 0x0001);			// transport stream id, 用来区别其他的TS流
	
	bits_write(&bits, 2, 0x03);				// reserved2, 固定为0x03
	bits_write(&bits, 5, 0x00); 			// version number, 范围0-31
	bits_write(&bits, 1, 1);				// current next indicator, 0 下一个表有效, 1当前传送的PAT表可以使用
	
	bits_write(&bits, 8, 0x00);				// section number, PAT可能分为多段传输，第一段为00
	bits_write(&bits, 8, 0x00);				// last section number
	
	bits_write(&bits, 16, 0x0001);			// program number
	bits_write(&bits, 3, 0x07);				// reserved3和pmt_pid是一组，共有几个频道由program number指示
	bits_write(&bits, 13, TS_PID_PMT);		// pmt of pid in ts head

	// char* test = "xyz";
	uint32_t crc32 = mpegCrc32(bits.p_data + 5, 12);

	// logError << "crc32: " << crc32;
 
	// bits_write(&bits, 32, crc32);				// CRC_32 先暂时写死
	bits_write(&bits, 8, crc32 & 0xff);				//CRC_32	先暂时写死
	bits_write(&bits, 8, (crc32 >> 8) & 0xff);
	bits_write(&bits, 8, (crc32 >> 16) & 0xff);
	bits_write(&bits, 8, (crc32 >> 24) & 0xff);
	// 每一个pat都会当成一个ts包来处理，所以每次剩余部分用1来充填完
	memset(buf + bits.i_data, 0xFF, TS_LOAD_LEN - bits.i_data);
	return TS_LOAD_LEN;
}

/* 
 *@remaark: 添加PMT头
 */
int TsMuxer::mk_ts_pmt_packet(char *buf, int handle)
{
	int nOffset = 0;
	int nRet = 0;
	
	bits_buffer_s bits;
	
	if (!buf)
	{
		return 0;
	}
    
    bits.i_size = 32; 
    bits.i_data = 0;
    bits.i_mask = 0x80; // 二进制：10000000 这里是为了后面对一个字节的每一位进行操作，避免大小端夸字节字序错乱
    bits.p_data = (unsigned char *)(buf);

	bits_write(&bits, 8, 0x47); //ts包起始字节
	bits_write(&bits, 1, 0);			// transport error indicator
	bits_write(&bits, 1, 1);		// payload unit start indicator
	bits_write(&bits, 1, 1);			// transport priority, 1表示高优先级
	bits_write(&bits, 13, TS_PID_PMT);	// pid, 0x00 PAT, 0x01 CAT
 
	bits_write(&bits, 2, 0);			// transport scrambling control, 传输加扰控制

	// continuity counter, 是具有同一PID值的TS包之间的连续计数值
	// 当分组的adaption_field_control字段为00话10时，该字段不递增
	bits_write(&bits, 2, 0x01);		// adaptation field control, 00 forbid, 01 have payload, 10 have adaptation, 11 have payload and adaptation
	bits_write(&bits, 4, _mapContinuity[TS_PID_PMT]); // continuity counter, 0~15
	_mapContinuity[TS_PID_PMT]++;
	_mapContinuity[TS_PID_PMT] &= 0x0F;

	bits_write(&bits, 8, 0x00); //因为前面负载开始标识为1，这里有一个调整字节,一般这个字节为0x00

	int sectionLength = 0x17;
	if (!_videoCodec || !_audioCodec) {
		sectionLength = 0x12;
	}

	// start pmt
	bits_write(&bits, 8, 0x02);				// table id, 固定为0x02
	
	bits_write(&bits, 1, 1);				// section syntax indicator, 固定为1
	bits_write(&bits, 1, 0);				// zero, 0
	bits_write(&bits, 2, 0x03);				// reserved1, 固定为0x03
	bits_write(&bits, 12, sectionLength);	 		// section length, 表示这个字节后面有用的字节数, 包括CRC32
	
	bits_write(&bits, 16, 0x0001);			// program number, 表示当前的PMT关联到的频道号码
	
	bits_write(&bits, 2, 0x03);				// reserved2, 固定为0x03
	bits_write(&bits, 5, 0x00); 			// version number, 范围0-31
	bits_write(&bits, 1, 1);				// current next indicator, 0 下一个表有效, 1当前传送的PAT表可以使用
	
	bits_write(&bits, 8, 0x00);				// section number, PAT可能分为多段传输，第一段为00
	bits_write(&bits, 8, 0x00);				// last section number
 
	bits_write(&bits, 3, 0x07);				// reserved3, 固定为0x07
	bits_write(&bits, 13, TS_PID_VIDEO);	// pcr of pid in ts head, 如果对于私有数据流的节目定义与PCR无关，这个域的值将为0x1FFF
	bits_write(&bits, 4, 0x0F);				// reserved4, 固定为0x0F
	bits_write(&bits, 12, 0x00);			// program info length, 前两位bit为00

	if (_videoCodec) {
		bits_write(&bits, 8, _videoCodec);		// stream type, 标志是Video还是Audio还是其他数据
		bits_write(&bits, 3, 0x07);				// reserved, 固定为0x07
		bits_write(&bits, 13, TS_PID_VIDEO);	// elementary of pid in ts head
		bits_write(&bits, 4, 0x0F);				// reserved, 固定为0x0F
		bits_write(&bits, 12, 0x00);			// elementary stream info length, 前两位bit为00
	}

	if (_audioCodec) {
		bits_write(&bits, 8, _audioCodec);		// stream type, 标志是Video还是Audio还是其他数据
		bits_write(&bits, 3, 0x07);				// reserved, 固定为0x07
		bits_write(&bits, 13, TS_PID_AUDIO);	// elementary of pid in ts head
		bits_write(&bits, 4, 0x0F);				// reserved, 固定为0x0F
		bits_write(&bits, 12, 0x00);			// elementary stream info length, 前两位bit为00
	}
 
	// bits_write(&bits, 8, 0xA4);				// stream type, 标志是Video还是Audio还是其他数据
	// bits_write(&bits, 3, 0x07);				// reserved, 固定为0x07
	// bits_write(&bits, 13, 0x00A4);			// elementary of pid in ts head
	// bits_write(&bits, 4, 0x0F);				// reserved, 固定为0x0F
	// bits_write(&bits, 12, 0x00);			// elementary stream info length, 前两位bit为00
 
	uint32_t crc32 = mpegCrc32(bits.p_data + 5, sectionLength - 4 + 3);
 
	// bits_write(&bits, 32, crc32);

	bits_write(&bits, 8, crc32 & 0xff);				//CRC_32	先暂时写死
	bits_write(&bits, 8, (crc32 >> 8) & 0xff);
	bits_write(&bits, 8, (crc32 >> 16) & 0xff);
	bits_write(&bits, 8, (crc32 >> 24) & 0xff);
	
	// 每一个pmt都会当成一个ts包来处理，所以每次剩余部分用1来充填完
	memset(buf + bits.i_data, 0xFF, TS_LOAD_LEN - bits.i_data);
	return TS_LOAD_LEN;
}

/* 
 *remark:添加pes头
 */
int TsMuxer::mk_pes_packet(char *buf, int bVideo, int length, int bDtsEn, unsigned long long pts, unsigned long long dts)
{
	pts *= 90;
    dts *= 90;

	// logInfo << "pts: ==================== " << pts;

	bits_buffer_s   bitsBuffer;
    bitsBuffer.i_size = 14;
    bitsBuffer.i_data = 0;
    bitsBuffer.i_mask = 0x80;
    bitsBuffer.p_data = (unsigned char *)(buf);
    memset(bitsBuffer.p_data, 0, 14);

	uint64_t payloadSize = length + 8;
	if (payloadSize > 0xffff) {
		// logWarn << "payloadSize > 0xffff: " << payloadSize;
		payloadSize = 0;
	}

    /*system header*/
    bits_write( &bitsBuffer, 24,0x000001);  /*start code*/
    bits_write( &bitsBuffer, 8, (bVideo ? 0xE0 : 0xC0)); /*streamID*/
    bits_write( &bitsBuffer, 16, payloadSize);  /*packet_len*/ //指出pes分组中数据长度和该字节后的长度和
    bits_write( &bitsBuffer, 2, 2 );    /*'10'*/
    bits_write( &bitsBuffer, 2, 0 );    /*scrambling_control*/
    bits_write( &bitsBuffer, 1, 0 );    /*priority*/
    bits_write( &bitsBuffer, 1, 0 );    /*data_alignment_indicator*/
    bits_write( &bitsBuffer, 1, 0 );    /*copyright*/
    bits_write( &bitsBuffer, 1, 0 );    /*original_or_copy*/
    bits_write( &bitsBuffer, 1, 1 );    /*PTS_flag*/
    bits_write( &bitsBuffer, 1, 0 );    /*DTS_flag*/
    bits_write( &bitsBuffer, 1, 0 );    /*ESCR_flag*/
    bits_write( &bitsBuffer, 1, 0 );    /*ES_rate_flag*/
    bits_write( &bitsBuffer, 1, 0 );    /*DSM_trick_mode_flag*/
    bits_write( &bitsBuffer, 1, 0 );    /*additional_copy_info_flag*/
    bits_write( &bitsBuffer, 1, 0 );    /*PES_CRC_flag*/
    bits_write( &bitsBuffer, 1, 0 );    /*PES_extension_flag*/
    bits_write( &bitsBuffer, 8, 5);    /*header_data_length*/ 
    // 指出包含在 PES 分组标题中的可选字段和任何填充字节所占用的总字节数。该字段之前
    //的字节指出了有无可选字段。
  
    /*PTS,DTS*/ 
    bits_write( &bitsBuffer, 4, 2 );                    /*'0011'*/
    bits_write( &bitsBuffer, 3, ((pts)>>30)&0x07 );     /*PTS[32..30]*/
    bits_write( &bitsBuffer, 1, 1 );
    bits_write( &bitsBuffer, 15,((pts)>>15)&0x7FFF);    /*PTS[29..15]*/
    bits_write( &bitsBuffer, 1, 1 );
    bits_write( &bitsBuffer, 15,(pts)&0x7FFF);          /*PTS[14..0]*/
    bits_write( &bitsBuffer, 1, 1 );
    // bits_write( &bitsBuffer, 4, 1 );                    /*'0001'*/
    // bits_write( &bitsBuffer, 3, ((dts)>>30)&0x07 );     /*DTS[32..30]*/
    // bits_write( &bitsBuffer, 1, 1 );
    // bits_write( &bitsBuffer, 15,((dts)>>15)&0x7FFF);    /*DTS[29..15]*/
    // bits_write( &bitsBuffer, 1, 1 );
    // bits_write( &bitsBuffer, 15,(dts)&0x7FFF);          /*DTS[14..0]*/
    // bits_write( &bitsBuffer, 1, 1 );

	return bitsBuffer.i_data;
}
