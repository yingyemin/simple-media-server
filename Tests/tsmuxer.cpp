
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
 
void bits_align(BITS_BUFFER_S *p_buffer)
{
	if (p_buffer->i_mask != 0x80 && p_buffer->i_data < p_buffer->i_size)
	{
		p_buffer->i_mask = 0x80;
		p_buffer->i_data++;
		p_buffer->p_data[p_buffer->i_data] = 0x00;
	}
}
 
inline void bits_write(BITS_BUFFER_S *p_buffer, int i_count, unsigned long i_bits)
{
	while (i_count > 0)
	{
		i_count--;
 
		if ((i_bits >> i_count ) & 0x01)
		{
			p_buffer->p_data[p_buffer->i_data] |= p_buffer->i_mask;
		}
		else
		{
			p_buffer->p_data[p_buffer->i_data] &= ~p_buffer->i_mask;
		}
		p_buffer->i_mask >>= 1;
		if (p_buffer->i_mask == 0)
		{
			p_buffer->i_data++;
			p_buffer->i_mask = 0x80;
		}
	}
}
 
 
int bits_initread(BITS_BUFFER_S *p_buffer, int i_size, unsigned char *p_data)
{
	if (!p_data)
	{
		return -1;
	}
	p_buffer->i_size = i_size;
	p_buffer->i_data = 0;
	p_buffer->i_mask = 0x80;
	p_buffer->p_data = p_data;
	return 0;
}
 
inline int bits_read(BITS_BUFFER_S *p_buffer, int i_count, unsigned long *i_bits)
{
	if (!i_bits)
	{
		return -1;
	}
	*i_bits = 0;
	
	while (i_count > 0)
	{
		i_count--;
 
		if (p_buffer->p_data[p_buffer->i_data] & p_buffer->i_mask)
		{
			*i_bits |= 0x01;
		}
 
		if (i_count)
		{
			*i_bits = *i_bits << 1;
		}
		
		p_buffer->i_mask >>= 1;
		if(p_buffer->i_mask == 0)
		{
			p_buffer->i_data++;
			p_buffer->i_mask = 0x80;
		}
	}
 
	return 0;
}

/*
 *@remark: 整体发送数据的抽象逻辑处理函数接口
 */
int rtsp_RTPPackage( RTP_SESSION_S *pRtpSender, int nFrameLen, StreamType_E enStreamType)
{
	int nRet = 0;
	int bVideo = 1 ;
	int nSendDataOff = 0;
	int nSendSize    = 0;	
	int nPayLoadSize = 0;
	int nHasSend     = 0;
	int IFrameFlag   = 0;
	char TSFrameHdr[1024];
	int nHead = 0;
	
 
	memset(TSFrameHdr, 0, 1024);
	memset(pRtpSender->stRtpPack, 0, RTP_MAX_PACKET_BUFF);
	bVideo = ((enStreamType == VIDEO_STREAM ) ? 1 : 0);
	
	// @remark: 判断数据是否为I帧，如果为I帧的话，加上PAT和PMT
	if( (bVideo == 1) && pRtpSender->stAvData.u8IFrame == 1)
	{
		if((nRet = mk_ts_pat_packet(TSFrameHdr +nSendDataOff, 
						pRtpSender->hHdlTs)) <= 0)	
		{
			DBG_INFO(" mk_ts_pat_packet failed!\n");
			return -1;
		}
		// @remark: 每次添加一个头的时候，必须注意指针到偏移量
		nSendDataOff += nRet;
		if((nRet = mk_ts_pmt_packet(TSFrameHdr + nSendDataOff, 
						pRtpSender->hHdlTs)) <= 0)	
		{
			DBG_INFO(" mk_ts_pmt_packet failed!\n");
			return -1;
		}
		nSendDataOff += nRet;
		
	}
	// @remark: 添加PS头，需要注意ps里也有一个计数的字段
	if((nRet = mk_ts_packet(TSFrameHdr + nSendDataOff, pRtpSender->hHdlTs, 
					1, bVideo, pRtpSender->stAvData.u8IFrame, pRtpSender->stAvData.u64TimeStamp)) <= 0 )
	{
		DBG_INFO(" mk_ts_packet failed!\n");
		return -1;
	}
	nSendDataOff  += nRet;
	//此字段是用来计算ts长度，因为ts包是固定188字节长度
	nHead = nRet;	
	
	// @remark: 添加PES头，后面就必须接H264数据了，不能通过1来填充
	if((nRet = mk_pes_packet(TSFrameHdr + nSendDataOff, bVideo, nFrameLen, 1, 
					pRtpSender->stAvData.u64TimeStamp, pRtpSender->stAvData.u64TimeStamp)) <= 0 )
	{
		DBG_INFO(" mk_pes_packet failed!\n");
		return -1;
	}
	nSendDataOff += nRet;	 
	nHead += nRet;
	
	// @remark: 如果第一次发送的数据长度大于剩余长度，则先发送ts包剩余长度的数据
	if( nFrameLen > (TS_LOAD_LEN - nHead))
	{
		memcpy(TSFrameHdr + nSendDataOff,  pRtpSender->stAvData.data, TS_LOAD_LEN - nHead);
		nSendDataOff += (TS_LOAD_LEN - nHead);
		nHasSend      = (TS_LOAD_LEN - nHead);
		if( rtsp_send_rtppack(TSFrameHdr, &nSendDataOff, pRtpSender->stAvData.u64TimeStamp, 0, (pRtpSender->stAvData.u8IFrame?1:0), bVideo, 1, pRtpSender) != 0 )
		{
			DBG_INFO(" rtsp_send_pack failed!\n");
			return -1;
		}
	}
	// @remark: 如果第一次发送数据长度小于ts头剩余长度，则，发送数据帧长度，剩余没有188长度的用1填充
	else 
	{
		memcpy(TSFrameHdr + nSendDataOff,  pRtpSender->stAvData.data, nFrameLen);
		nSendDataOff += nFrameLen;
		nHasSend      = nFrameLen;
		memset(TSFrameHdr +nSendDataOff, 0xFF, (TS_LOAD_LEN-nHead - nFrameLen));
		nSendDataOff += (TS_LOAD_LEN -nHead- nFrameLen);
		if( rtsp_send_rtppack(TSFrameHdr, &nSendDataOff, pRtpSender->stAvData.u64TimeStamp, 1, (pRtpSender->stAvData.u8IFrame?1:0), bVideo, 1, pRtpSender) != 0 )
		{
			DBG_INFO(" rtsp_send_rtppack failed!\n");
			return -1;
		}
	}
		
 
	// 对应的数据便宜长度，因为我处理的时候时固定1460到rtp包发送数据，所以这里会处理偏移，方便添加rtp头
	nPayLoadSize = RTP_MAX_PACKET_BUFF - 4 - RTP_HDR_LEN -  (4+6) * 7; // 减去rtp头，ts头 ,一个rtp包最多7个ts包
	nFrameLen -= (TS_LOAD_LEN - nHead);
 
	// @remark: 第二次发送数据了，此时发送数据时候，就需要外再添加ps头了
	while(nFrameLen > 0 )
	{
 
		nSendSize = (nFrameLen > nPayLoadSize) ? nPayLoadSize : nFrameLen;
		if( rtsp_send_rtppack(pRtpSender->stAvData.data + nHasSend, &nSendSize, pRtpSender->stAvData.u64TimeStamp, 
					((nSendSize == nFrameLen) ? 1 : 0),  IFrameFlag, bVideo, 0, pRtpSender) != 0 )
		{
			DBG_INFO(" rtsp_send_rtppack failed!\n");
			return -1;
		}
		nFrameLen -= nSendSize;
		nHasSend  += nSendSize;
		memset(pRtpSender->stRtpPack, 0, RTP_MAX_PACKET_BUFF);
		IFrameFlag = 0;
	}
	return 0;
}

/* 
 *@remark : 添加pat头 
 */
int mk_ts_pat_packet(char *buf, int handle)
{
	int nOffset = 0;
	int nRet = 0;
	
	if (!buf)
	{
		return 0;
	}
 
	if (0 >= (nRet = ts_header(buf, handle, TS_TYPE_PAT, 1)))
	{
		return 0;
	}
	nOffset += nRet;
 
	if (0 >= (nRet = ts_pointer_field(buf + nOffset)))
	{
		return 0;
	}
	nOffset += nRet;
	
	if (0 >= (nRet = ts_pat_header(buf + nOffset)))
	{
		return 0;
	}
	nOffset += nRet;
 
	// 每一个pat都会当成一个ts包来处理，所以每次剩余部分用1来充填完
	memset(buf + nOffset, 0xFF, TS_PACKET_SIZE - nOffset);
	return TS_PACKET_SIZE;
}

int ts_pat_header(char *buf)
{
	BITS_BUFFER_S bits;
	
	if (!buf)
	{
		return 0;
	}
	bits_initwrite(&bits, 32, (unsigned char *)buf);
 
	bits_write(&bits, 8, 0x00); 			// table id, 固定为0x00 
	
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
 
	bits_write(&bits, 8, 0x9F);				// CRC_32 先暂时写死
	bits_write(&bits, 8, 0xC7);
	bits_write(&bits, 8, 0x62);
	bits_write(&bits, 8, 0x58);
 
	bits_align(&bits);
	return bits.i_data;
}

/* 
 *@remaark: 添加PMT头
 */
int mk_ts_pmt_packet(char *buf, int handle)
{
	int nOffset = 0;
	int nRet = 0;
	
	if (!buf)
	{
		return 0;
	}
 
	if (0 >= (nRet = ts_header(buf, handle, TS_TYPE_PMT, 1)))
	{
		return 0;
	}
	nOffset += nRet;
 
	if (0 >= (nRet = ts_pointer_field(buf + nOffset)))
	{
		return 0;
	}
	nOffset += nRet;
 
	if (0 >= (nRet = ts_pmt_header(buf + nOffset)))
	{
		return 0;
	}
	nOffset += nRet;
 
	// 每一个pmt都会当成一个ts包来处理，所以每次剩余部分用1来充填完
	memset(buf + nOffset, 0xFF, TS_PACKET_SIZE - nOffset);
	return TS_PACKET_SIZE;
}

int ts_pmt_header(char *buf)
{
	BITS_BUFFER_S bits;
 
	if (!buf)
	{
		return 0;
	}
 
	bits_initwrite(&bits, 32, (unsigned char *)buf);
 
	bits_write(&bits, 8, 0x02);				// table id, 固定为0x02
	
	bits_write(&bits, 1, 1);				// section syntax indicator, 固定为1
	bits_write(&bits, 1, 0);				// zero, 0
	bits_write(&bits, 2, 0x03);				// reserved1, 固定为0x03
	bits_write(&bits, 12, 0x1C);	 		// section length, 表示这个字节后面有用的字节数, 包括CRC32
	
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
 
	bits_write(&bits, 8, TS_PMT_STREAMTYPE_H264_VIDEO);		// stream type, 标志是Video还是Audio还是其他数据
	bits_write(&bits, 3, 0x07);				// reserved, 固定为0x07
	bits_write(&bits, 13, TS_PID_VIDEO);	// elementary of pid in ts head
	bits_write(&bits, 4, 0x0F);				// reserved, 固定为0x0F
	bits_write(&bits, 12, 0x00);			// elementary stream info length, 前两位bit为00
 
	bits_write(&bits, 8, TS_PMT_STREAMTYPE_11172_AUDIO);		// stream type, 标志是Video还是Audio还是其他数据
	bits_write(&bits, 3, 0x07);				// reserved, 固定为0x07
	bits_write(&bits, 13, TS_PID_AUDIO);	// elementary of pid in ts head
	bits_write(&bits, 4, 0x0F);				// reserved, 固定为0x0F
	bits_write(&bits, 12, 0x00);			// elementary stream info length, 前两位bit为00
 
	bits_write(&bits, 8, 0xA4);				// stream type, 标志是Video还是Audio还是其他数据
	bits_write(&bits, 3, 0x07);				// reserved, 固定为0x07
	bits_write(&bits, 13, 0x00A4);			// elementary of pid in ts head
	bits_write(&bits, 4, 0x0F);				// reserved, 固定为0x0F
	bits_write(&bits, 12, 0x00);			// elementary stream info length, 前两位bit为00
 
	bits_write(&bits, 8, 0x34);				//CRC_32	先暂时写死
	bits_write(&bits, 8, 0x12);
	bits_write(&bits, 8, 0xA3);
	bits_write(&bits, 8, 0x72);
	
	bits_align(&bits);
	return bits.i_data;
}

/* 
 *@remark: ts头的封装
 */
int mk_ts_packet(char *buf, int handle, int bStart, int bVideo, int bIFrame, unsigned long long timestamp)
{
	int nOffset = 0;
	int nRet = 0;
	
	if (!buf)
	{
		return 0;
	}
 
	if (0 >= (nRet = ts_header(buf, handle, bVideo ? TS_TYPE_VIDEO : TS_TYPE_AUDIO, bStart)))
	{
		return 0;
	}
	nOffset += nRet;
 
	if (0 >= (nRet = ts_adaptation_field(buf + nOffset, bStart, bVideo && (bIFrame), timestamp)))
	{
		return 0;
	}
	nOffset += nRet;
 
	return nOffset;
}
 
/* *@remark: ts头相关封装
 *  PSI 包括了PAT、PMT、NIT、CAT
 *  PSI--Program Specific Information, PAT--program association table, PMT--program map table
 *  NIT--network information table, CAT--Conditional Access Table
 *  一个网络中可以有多个TS流(用PAT中的ts_id区分)
 *  一个TS流中可以有多个频道(用PAT中的pnumber、pmt_pid区分)
 *  一个频道中可以有多个PES流(用PMT中的mpt_stream区分)
 */
int ts_header(char *buf, int handle, TS_TYPE_E type, int bStart)
{
	BITS_BUFFER_S bits;
	TS_MNG_S *pMng = (TS_MNG_S *)handle;
 
	if (!buf || !handle || TS_TYPE_BEGIN >= type || TS_TYPE_END <= type)
	{
		return 0;
	}
 
	bits_initwrite(&bits, 32, (unsigned char *)buf);
 
	bits_write(&bits, 8, 0x47);			// sync_byte, 固定为0x47,表示后面的是一个TS分组
 
	// payload unit start indicator根据TS packet究竟是包含PES packet还是包含PSI data而设置不同值
	// 1. 若包含的是PES packet header, 设为1,  如果是PES packet余下内容, 则设为0
	// 2. 若包含的是PSI data, 设为1, 则payload的第一个byte将是point_field, 0则表示payload中没有point_field
	// 3. 若此TS packet为null packet, 此flag设为0
	bits_write(&bits, 1, 0);			// transport error indicator
	bits_write(&bits, 1, bStart);		// payload unit start indicator
	bits_write(&bits, 1, 0);			// transport priority, 1表示高优先级
	if (TS_TYPE_PAT == type)
	{
		bits_write(&bits, 13, 0x00);	// pid, 0x00 PAT, 0x01 CAT
	}
	else if (TS_TYPE_PMT == type)
	{
		bits_write(&bits, 13, TS_PID_PMT);
	}
	else if (TS_TYPE_VIDEO == type)
	{
		bits_write(&bits, 13, TS_PID_VIDEO);
	}
	else if (TS_TYPE_AUDIO == type)
	{
		bits_write(&bits, 13, TS_PID_AUDIO);
	}
 
	bits_write(&bits, 2, 0);			// transport scrambling control, 传输加扰控制
	if (TS_TYPE_PAT == type || TS_TYPE_PMT == type)
	{
		// continuity counter, 是具有同一PID值的TS包之间的连续计数值
		// 当分组的adaption_field_control字段为00话10时，该字段不递增
		bits_write(&bits, 2, 0x01);		// adaptation field control, 00 forbid, 01 have payload, 10 have adaptation, 11 have payload and adaptation
		bits_write(&bits, 4, pMng->nPatCounter);	// continuity counter, 0~15
		
		if (TS_TYPE_PAT != type)
		{
			pMng->nPatCounter++;
			pMng->nPatCounter &= 0x0F;
		}
	}
	else
	{
		bits_write(&bits, 2, 0x03);		// 第一位表示有无调整字段，第二位表示有无有效负载
		bits_write(&bits, 4, pMng->nContinuityCounter);
		pMng->nContinuityCounter++;
		pMng->nContinuityCounter &= 0x0F;
	}
	
	bits_align(&bits);
	return bits.i_data;
}

/* 
 *remark:添加pes头
 */
int mk_pes_packet(char *buf, int bVideo, int length, int bDtsEn, unsigned long long pts, unsigned long long dts)
{
	PES_HEAD_S pesHead;
	PES_OPTION_S pesOption;
	PES_PTS_S pesPts;
	PES_PTS_S pesDts;
 
	if (!buf)
	{
		return 0;
	}
 
	if( bVideo == 1)
	{
		// 视频的采样频率为90kHZ,则增量为3600
		pts = pts * 9 / 100;	//  90000Hz
		dts = dts * 9 / 100;	//  90000Hz 
	}	
	else 
	{
		// 音频的话，则需要按照8000HZ来计算增量[需要的话]
		pts = pts * 8 / 1000;	// 8000Hz
		dts = dts * 8 / 1000;	// 8000Hz 
		
	}
 
	memset(&pesHead, 0, sizeof(pesHead));
	memset(&pesOption, 0, sizeof(pesOption));
	memset(&pesPts, 0, sizeof(pesPts));
	memset(&pesDts, 0, sizeof(pesDts));
 
	pesHead.startcode = htonl(0x000001) >> 8;
	pesHead.stream_id = bVideo ? 0xE0 : 0xC0;
	if (PES_MAX_SIZE < length)
	{
		pesHead.pack_len = 0;
	}
	else
	{
		pesHead.pack_len = htons(length + sizeof(pesOption) + sizeof(pesPts) + (bDtsEn ? sizeof(pesDts) : 0));
	}
 
	pesOption.fixed = 0x02;
	pesOption.pts_dts = bDtsEn ? 0x03 : 0x02;
	pesOption.head_len = sizeof(pesPts) + (bDtsEn ? sizeof(pesDts) : 0);
 
	pesPts.fixed2 = pesPts.fixed3 = pesPts.fixed4 = 0x01;
	pesPts.fixed1 = bDtsEn ? 0x03 : 0x02;
	pesPts.ts1 = (pts >> 30) & 0x07;
	pesPts.ts2 = (pts >> 22) & 0xFF;
	pesPts.ts3 = (pts >> 15) & 0x7F;
	pesPts.ts4 = (pts >> 7) & 0xFF;
	pesPts.ts5 = pts & 0x7F;
	
	pesDts.fixed1 = pesDts.fixed2 = pesDts.fixed3 = pesDts.fixed4 = 0x01;
	pesDts.ts1 = (dts >> 30) & 0x07;
	pesDts.ts2 = (dts >> 22) & 0xFF;
	pesDts.ts3 = (dts >> 15) & 0x7F;
	pesDts.ts4 = (dts >> 7) & 0xFF;
	pesDts.ts5 = dts & 0x7F;
 
	char *head = buf;
	memcpy(head, &pesHead, sizeof(pesHead));
	head += sizeof(pesHead);
	memcpy(head, &pesOption, sizeof(pesOption));
	head += sizeof(pesOption);
	memcpy(head, &pesPts, sizeof(pesPts));
	head += sizeof(pesPts);
	if (bDtsEn)
	{
		memcpy(head, &pesDts, sizeof(pesDts));
		head += sizeof(pesPts);
	}
	
	return (head - buf);
}

/* 
 *@remark: 最后封装rtp头并发送最终封装好到完整的数据包
 */
int rtsp_send_rtppack(char *Databuf, int *datalen, unsigned long curtimestamp, int mark_flag, int IFrameFlag, int bVideo, int nFrameStart, RTP_SESSION_S *pRtpSender)
{
	int nHasSend     = 0;
	int nRet		 = 0;	
	int nTsHeadNum   = 0;
	int nHadDataLen  = 0;
	int nTcpSendLen  = 0;
	static unsigned short cSeqnum;
 
 
	// @remark：表示为数据的第一次发送，所以不需要额外再添加ts头
	if( nFrameStart == 1 )
	{
		nRet = mk_rtp_packet(pRtpSender->stRtpPack + nHasSend, mark_flag, IFrameFlag, bVideo, ++cSeqnum, (curtimestamp * 9/100)); 
		nHasSend += nRet;
		memcpy(pRtpSender->stRtpPack + nHasSend, Databuf, *datalen);
		nHasSend += *datalen;
	}
	else  // 不是第一次发送此帧数据的话，则需要添加封装新的ts包，并添加ts头
	{
		// rtp+ rtp_ext + ts  +data 
		nRet = mk_rtp_packet(pRtpSender->stRtpPack + nHasSend, mark_flag, IFrameFlag, bVideo, ++cSeqnum, (curtimestamp * 9/100)); 
		nHasSend += nRet;
		while(*datalen > 0 && nTsHeadNum < 7)
		{
			nRet = mk_ts_packet(pRtpSender->stRtpPack + nHasSend , pRtpSender->hHdlTs, 0, bVideo, (IFrameFlag > 0 ? 1:0), curtimestamp);
			nHasSend += nRet;
			if(*datalen < (TS_LOAD_LEN- nRet))
			{
				memcpy(pRtpSender->stRtpPack + nHasSend, Databuf + nHadDataLen, *datalen);
				nHasSend	+= *datalen;
				nHadDataLen += *datalen;	
			
				//不够Ts188用1补充	
				memset(pRtpSender->stRtpPack + nHasSend, 0xFF, TS_LOAD_LEN- nRet - (*datalen));
				nHasSend += (TS_LOAD_LEN - nRet - *datalen);
			}	
			else 
			{
				memcpy(pRtpSender->stRtpPack + nHasSend, Databuf + nHadDataLen, TS_LOAD_LEN - nRet);
				nHasSend    += (TS_LOAD_LEN - nRet);
				*datalen    -= (TS_LOAD_LEN - nRet);
				nHadDataLen += (TS_LOAD_LEN - nRet);	
			}
			nTsHeadNum ++; 
		}
		*datalen = nHadDataLen; //实际发送裸数据到长度
	}
 
 
	if(pRtpSender->RtspsockFd <= 0 )
	{
		DBG_INFO("send rtp packet socket error\n");	
		return -1;
	}
	
	nTcpSendLen = hi_tcp_noblock_send(pRtpSender->RtspsockFd, pRtpSender->stRtpPack, nHasSend, NULL,1500); 
	if(nTcpSendLen != nHasSend )
	{
		DBG_INFO("send rtp packet failed:%s\n",strerror(errno));	
		return -1;
	}
	return 0;
}