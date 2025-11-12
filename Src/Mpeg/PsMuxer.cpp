#include <string>
#include <unordered_map>

#include "Common/Track.h"
#include "Common/Frame.h"
#include "Net/Buffer.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"
#include "PsMuxer.h"
#include "Log/Logger.h"
#include "Mpeg.h"

using namespace std;

PsMuxer::PsMuxer()
{
    logInfo << "PsMuxer";
}

PsMuxer::~PsMuxer()
{
    logInfo << "~PsMuxer";
}

void PsMuxer::onFrame(const FrameBuffer::Ptr& frame)
{
    if (!_startEncode || !frame) {
        return ;
    }

    // if (frame->getTrackType() == VideoTrackType) {
    //     logInfo << "nal type: " << (int)frame->getNalType() << ", pts: " << frame->pts();
    //     FILE* fp = fopen("testmux.h266", "ab+");
    //     fwrite(frame->data(), 1, frame->size(), fp);
    //     fclose(fp);
    // }

    // framesouce 里已经添加了sps等帧，这里不再重复添加
    // if (frame->metaFrame()) {
    //     _sendMetaFrame = true;
    // }

    // if (frame->keyFrame()) {
    //     if (!_sendMetaFrame) {
    //         auto track = _mapTrackInfo[frame->getTrackIndex()];
    //         if (track->codec_ == "h264") {
    //             auto h264Track = dynamic_pointer_cast<H264Track>(track);
    //             encode(h264Track->_sps);
    //             encode(h264Track->_pps);
    //         } else if (track->codec_ == "h265") {
    //             auto h265Track = dynamic_pointer_cast<H265Track>(track);
    //             encode(h265Track->_vps);
    //             encode(h265Track->_sps);
    //             encode(h265Track->_pps);
    //         }
    //     } else {
    //         _sendMetaFrame = false;
    //     }
    // }
    
    // _mapStampAdjust[frame->_index]->inputStamp(frame->_pts, frame->_dts, 1);
    encode(frame);
}

void PsMuxer::startEncode()
{
    _startEncode = true;
}

void PsMuxer::stopEncode()
{
    _startEncode = false;
}

void PsMuxer::addTrackInfo(const shared_ptr<TrackInfo>& trackInfo)
{
    if (!trackInfo) {
        throw runtime_error("trackInfo is null");
        return ;
    }

    _mapTrackInfo[trackInfo->index_] = trackInfo;
    if (trackInfo->trackType_ == "video") {
        _mapStampAdjust[trackInfo->index_] = make_shared<VideoStampAdjust>(25);
        _mapStreamId[trackInfo->index_] = _lastVideoId++;
        if (trackInfo->codec_ == "h264") {
            _videoCodec = STREAM_TYPE_VIDEO_H264;
        } else if (trackInfo->codec_ == "h265") {
            _videoCodec = STREAM_TYPE_VIDEO_HEVC;
        } else if (trackInfo->codec_ == "vp8") {
            _videoCodec = STREAM_TYPE_VIDEO_VP8;
        } else if (trackInfo->codec_ == "vp9") {
            _videoCodec = STREAM_TYPE_VIDEO_VP9;
        } else if (trackInfo->codec_ == "av1") {
            _videoCodec = STREAM_TYPE_VIDEO_AV1;
        } else if (trackInfo->codec_ == "h266") {
            _videoCodec = STREAM_TYPE_VIDEO_H266;
        } else {
            throw runtime_error("unsupport video codec: " + trackInfo->codec_);
        }
    } else if (trackInfo->trackType_ == "audio") {
        _mapStampAdjust[trackInfo->index_] = make_shared<AudioStampAdjust>(0);
        _mapStreamId[trackInfo->index_] = _lastAudioId++;
        if (trackInfo->codec_ == "aac") {
            _audioCodec = STREAM_TYPE_AUDIO_AAC;
        } else if (trackInfo->codec_ == "g711a") {
            _audioCodec = STREAM_TYPE_AUDIO_G711;
        } else if (trackInfo->codec_ == "g711u") {
            _audioCodec = STREAM_TYPE_AUDIO_G711ULAW;
        } else if (trackInfo->codec_ == "mp3") {
            _audioCodec = STREAM_TYPE_AUDIO_MP3;
        } else if (trackInfo->codec_ == "opus") {
            _audioCodec = STREAM_TYPE_AUDIO_OPUS;
        } else {
            throw runtime_error("unsupport audio codec: " + trackInfo->codec_);
        }
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
 
int PsMuxer::encode(const FrameBuffer::Ptr& frame)
{
    if (!frame) {
        return 0;
    }
    _psFrame = make_shared<FrameBuffer>();
    // _psFrame->_buffer.resize(256);

    int nSizePos = 0;
    // 1 package for ps header 
    nSizePos += makePsHeader(frame->pts(), nSizePos);
    // nSizePos += PS_HDR_LEN; 

    //2 system header 
    // if( frame->keyFrame() )
    // {
        // 如果是I帧的话，则添加系统头
        nSizePos += makeSysHeader(nSizePos);
        // nSizePos += SYS_HDR_LEN;
        // psm头 (也是map)
        nSizePos += makePsmHeader(nSizePos);
        // nSizePos += PSM_HDR_LEN;
    // }
 
    auto pBuff = frame->data();
    int nFrameLen = frame->size();
    int nSize = 0;

    int trackIndex = frame->getTrackIndex();
    if (_mapStreamId.find(trackIndex) == _mapStreamId.end()) {
        return -1;
    }
    auto streamId = _mapStreamId[frame->getTrackIndex()];
    while (nFrameLen > 0)
    {
        //每次帧的长度不要超过short类型，过了就得分片进循环行发送
        nSize = (nFrameLen > PS_PES_PAYLOAD_SIZE) ? PS_PES_PAYLOAD_SIZE : nFrameLen;
        // 添加pes头
        nSizePos += makePesHeader(streamId, nSize, frame->pts(), frame->dts(), nSizePos);

        _psFrame->_buffer->append(pBuff, nSize);
        if (_onPsFrame) {
            _psFrame->_isKeyframe = frame->metaFrame();
            _onPsFrame(_psFrame);
        }

        _psFrame = make_shared<FrameBuffer>();
        // _psFrame->_buffer.resize(256);

        //分片后每次发送的数据移动指针操作
        nFrameLen -= nSize;
        //这里也只移动nSize,因为在while向后移动的pes头长度，正好重新填充pes头数据
        pBuff     += nSize;
        nSizePos  += nSize;
    }
    return 0;
}
 
/***
 *@remark:   ps头的封装,里面的具体数据的填写已经占位，可以参考标准
 *@param :   pData  [in] 填充ps头数据的地址
 *           s64Src [in] 时间戳
 *@return:   0 success, others failed
*/
int PsMuxer::makePsHeader(int dts, int index)
{
    // auto pData = _psFrame->data() + index;
    unsigned char pData[PS_HDR_LEN] = {0};
    dts *= 90;
    // lScrExt 的时钟是27000000，dts的是90000，因此要乘以300
    // 这里lScrExt肯定是0了
    unsigned long long lScrExt = (dts * 300) % 100;
 
    // 这里除以100是由于sdp协议返回的video的频率是90000，帧率是25帧/s，所以每次递增的量是3600,
    // 所以实际你应该根据你自己编码里的时间戳来处理以保证时间戳的增量为3600即可，
    //如果这里不对的话，就可能导致卡顿现象了
    bits_buffer_s   bitsBuffer;
    bitsBuffer.i_size = PS_HDR_LEN; 
    bitsBuffer.i_data = 0;
    bitsBuffer.i_mask = 0x80; // 二进制：10000000 这里是为了后面对一个字节的每一位进行操作，避免大小端夸字节字序错乱
    bitsBuffer.p_data = (unsigned char *)(pData);
    memset(bitsBuffer.p_data, 0, PS_HDR_LEN);
    bits_write(&bitsBuffer, 32, 0x000001BA);      /*start codes*/
    bits_write(&bitsBuffer, 2,  1);           /*marker bits '01b'*/
    bits_write(&bitsBuffer, 3,  (dts>>30)&0x07);     /*System clock [32..30]*/
    bits_write(&bitsBuffer, 1,  1);           /*marker bit*/
    bits_write(&bitsBuffer, 15, (dts>>15)&0x7FFF);   /*System clock [29..15]*/
    bits_write(&bitsBuffer, 1,  1);           /*marker bit*/
    bits_write(&bitsBuffer, 15, dts&0x7fff);         /*System clock [14..0]*/
    bits_write(&bitsBuffer, 1,  1);           /*marker bit*/
    bits_write(&bitsBuffer, 9,  lScrExt&0x01ff);    /*System clock ext*/
    bits_write(&bitsBuffer, 1,  1);           /*marker bit*/
    bits_write(&bitsBuffer, 22, (255)&0x3fffff);    /*bit rate(n units of 50 bytes per second.)*/
    bits_write(&bitsBuffer, 2,  3);           /*marker bits '11'*/
    bits_write(&bitsBuffer, 5,  0x1f);          /*reserved(reserved for future use)*/
    bits_write(&bitsBuffer, 3,  0);           /*stuffing length*/

    _psFrame->_buffer->append((char*)pData, PS_HDR_LEN);
    
    return PS_HDR_LEN;
}
 
/***
 *@remark:   sys头的封装,里面的具体数据的填写已经占位，可以参考标准
 *@param :   pData  [in] 填充ps头数据的地址
 *@return:   0 success, others failed
*/
int PsMuxer::makeSysHeader(int index)
{
    // auto pData = _psFrame->data() + index;
    unsigned char pData[SYS_HDR_LEN] = {0};

    bits_buffer_s   bitsBuffer;
    bitsBuffer.i_size = SYS_HDR_LEN;
    bitsBuffer.i_data = 0;
    bitsBuffer.i_mask = 0x80;
    bitsBuffer.p_data = (unsigned char *)(pData);
    memset(bitsBuffer.p_data, 0, SYS_HDR_LEN);
    /*system header*/
    bits_write( &bitsBuffer, 32, 0x000001BB); /*start code*/
    bits_write( &bitsBuffer, 16, SYS_HDR_LEN-6);/*header_length 表示次字节后面的长度，后面的相关头也是次意思*/
    bits_write( &bitsBuffer, 1,  1);            /*marker_bit*/
    bits_write( &bitsBuffer, 22, 50000);    /*rate_bound*/
    bits_write( &bitsBuffer, 1,  1);            /*marker_bit*/
    bits_write( &bitsBuffer, 6,  1);            /*audio_bound*/
    bits_write( &bitsBuffer, 1,  0);            /*fixed_flag */
    bits_write( &bitsBuffer, 1,  1);          /*CSPS_flag */
    bits_write( &bitsBuffer, 1,  1);          /*system_audio_lock_flag*/
    bits_write( &bitsBuffer, 1,  1);          /*system_video_lock_flag*/
    bits_write( &bitsBuffer, 1,  1);          /*marker_bit*/
    bits_write( &bitsBuffer, 5,  1);          /*video_bound*/
    bits_write( &bitsBuffer, 1,  0);          /*dif from mpeg1*/
    bits_write( &bitsBuffer, 7,  0x7F);       /*reserver*/
    /*audio stream bound*/
    bits_write( &bitsBuffer, 8,  0xC0);         /*stream_id*/
    bits_write( &bitsBuffer, 2,  3);          /*marker_bit */
    bits_write( &bitsBuffer, 1,  0);            /*PSTD_buffer_bound_scale*/
    bits_write( &bitsBuffer, 13, 512);          /*PSTD_buffer_size_bound*/
    /*video stream bound*/
    bits_write( &bitsBuffer, 8,  0xE0);         /*stream_id*/
    bits_write( &bitsBuffer, 2,  3);          /*marker_bit */
    bits_write( &bitsBuffer, 1,  1);          /*PSTD_buffer_bound_scale*/
    bits_write( &bitsBuffer, 13, 2048);       /*PSTD_buffer_size_bound*/

    _psFrame->_buffer->append((char*)pData, SYS_HDR_LEN);
    
    return SYS_HDR_LEN;
}
 
/***
 *@remark:   psm头的封装,里面的具体数据的填写已经占位，可以参考标准
 *@param :   pData  [in] 填充ps头数据的地址
 *@return:   0 success, others failed
*/
int PsMuxer::makePsmHeader(int index)
{
    int bothFlag = true;
    int dataLen = PSM_HDR_LEN;
    if (!_videoCodec || !_audioCodec) {
        bothFlag = false;
        dataLen -= 4;
    }
    // auto pData = _psFrame->data() + index;
    unsigned char pData[PSM_HDR_LEN] = {0};

    bits_buffer_s   bitsBuffer;
    bitsBuffer.i_size = dataLen; 
    bitsBuffer.i_data = 0;
    bitsBuffer.i_mask = 0x80;
    bitsBuffer.p_data = (unsigned char *)(pData);
    memset(bitsBuffer.p_data, 0, dataLen);
    bits_write(&bitsBuffer, 24,0x000001); /*start code*/
    bits_write(&bitsBuffer, 8, 0xBC);   /*map stream id*/
    bits_write(&bitsBuffer, 16,dataLen - 6);     /*program stream map length*/ 
    bits_write(&bitsBuffer, 1, 1);      /*current next indicator */
    bits_write(&bitsBuffer, 2, 3);      /*reserved*/
    bits_write(&bitsBuffer, 5, 0);      /*program stream map version*/
    bits_write(&bitsBuffer, 7, 0x7F);   /*reserved */
    bits_write(&bitsBuffer, 1, 1);      /*marker bit */
    bits_write(&bitsBuffer, 16,0);      /*programe stream info length*/
    if (bothFlag) {
        bits_write(&bitsBuffer, 16, 8);     /*elementary stream map length  is*/
    } else {
        bits_write(&bitsBuffer, 16, 4);     /*elementary stream map length  is*/
    }
    /*audio*/
    if (_audioCodec) {
        bits_write(&bitsBuffer, 8, _audioCodec);       /*stream_type*/
        bits_write(&bitsBuffer, 8, 0xC0);   /*elementary_stream_id*/
        bits_write(&bitsBuffer, 16, 0);     /*elementary_stream_info_length is*/
    }
    /*video*/
    if (_videoCodec) {
        bits_write(&bitsBuffer, 8, _videoCodec);       /*stream_type*/
        bits_write(&bitsBuffer, 8, 0xE0);   /*elementary_stream_id*/
        bits_write(&bitsBuffer, 16, 0);     /*elementary_stream_info_length */
    }
    /*crc (2e b9 0f 3d)*/
    bits_write(&bitsBuffer, 8, 0x45);   /*crc (24~31) bits*/
    bits_write(&bitsBuffer, 8, 0xBD);   /*crc (16~23) bits*/
    bits_write(&bitsBuffer, 8, 0xDC);   /*crc (8~15) bits*/
    bits_write(&bitsBuffer, 8, 0xF4);   /*crc (0~7) bits*/

    // if (bothFlag) {
        _psFrame->_buffer->append((char*)pData, dataLen);
        return dataLen; 
    // } else {
    //     _psFrame->_buffer.append((char*)pData, PSM_HDR_LEN - 4);
    //     return PSM_HDR_LEN - 4;
    // }
}
 
/***
 *@remark:   pes头的封装,里面的具体数据的填写已经占位，可以参考标准
 *@param :   pData      [in] 填充ps头数据的地址
 *           stream_id  [in] 码流类型
 *           paylaod_len[in] 负载长度
 *           pts        [in] 时间戳
 *           dts        [in]
 *@return:   0 success, others failed
*/
int PsMuxer::makePesHeader(int stream_id, int payload_len, 
                                unsigned long long pts, unsigned long long dts, int nSizePos)
{
    // auto pData = _psFrame->data() + nSizePos;
    unsigned char pData[PES_HDR_LEN] = {0};

    pts *= 90;
    dts *= 90;

    bits_buffer_s   bitsBuffer;
    bitsBuffer.i_size = PES_HDR_LEN;
    bitsBuffer.i_data = 0;
    bitsBuffer.i_mask = 0x80;
    bitsBuffer.p_data = (unsigned char *)(pData);
    memset(bitsBuffer.p_data, 0, PES_HDR_LEN);
    /*system header*/
    bits_write( &bitsBuffer, 24,0x000001);  /*start code*/
    bits_write( &bitsBuffer, 8, (stream_id)); /*streamID*/
    bits_write( &bitsBuffer, 16,(payload_len)+8);  /*packet_len*/ //指出pes分组中数据长度和该字节后的长度和
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

    _psFrame->_buffer->append((char*)pData, PES_HDR_LEN);

    return PES_HDR_LEN;
}