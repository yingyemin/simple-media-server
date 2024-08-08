#ifndef Mpeg_H
#define Mpeg_H

#include <stdint.h>

#define PS_AUDIO_ID 0xc0
#define PS_AUDIO_ID_END 0xdf
#define PS_VIDEO_ID 0xe0
#define PS_VIDEO_ID_END 0xef

#define STREAM_TYPE_RESERVED        0x00
#define STREAM_TYPE_VIDEO_MPEG1     0x01
#define STREAM_TYPE_VIDEO_MPEG2     0x02
#define STREAM_TYPE_AUDIO_MPEG1     0x03
#define STREAM_TYPE_AUDIO_MPEG2     0x04
#define STREAM_TYPE_PRIVATE_SECTION 0x05
#define STREAM_TYPE_PRIVATE_DATA    0x06
#define STREAM_TYPE_AUDIO_AAC       0x0f
#define STREAM_TYPE_VIDEO_MPEG4     0x10
#define STREAM_TYPE_VIDEO_H264      0x1b
#define STREAM_TYPE_VIDEO_HEVC      0x24
#define STREAM_TYPE_VIDEO_CAVS      0x42
#define STREAM_TYPE_VIDEO_SAVC      0x80

#define STREAM_TYPE_AUDIO_AC3       0x81

#define STREAM_TYPE_AUDIO_G711      0x90
#define STREAM_TYPE_AUDIO_G711ULAW  0x91
#define STREAM_TYPE_AUDIO_G722_1    0x92
#define STREAM_TYPE_AUDIO_G723_1    0x93
#define STREAM_TYPE_AUDIO_G726      0x96
#define STREAM_TYPE_AUDIO_G729_1    0x99
#define STREAM_TYPE_AUDIO_SVAC      0x9b
#define STREAM_TYPE_AUDIO_PCM       0x9c

/***
 *@remark:  讲传入的数据按地位一个一个的压入数据
 *@param :  buffer   [in]  压入数据的buffer
 *          count    [in]  需要压入数据占的位数
 *          bits     [in]  压入的数值
 */
#define bits_write(buffer, count, bits)\
{\
    bits_buffer_s *p_buffer = (buffer);\
    int i_count = (count);\
    uint64_t i_bits = (bits);\
    while( i_count > 0 )\
    {\
        i_count--;\
        if( ( i_bits >> i_count )&0x01 )\
        {\
            p_buffer->p_data[p_buffer->i_data] |= p_buffer->i_mask;\
        }\
        else\
        {\
            p_buffer->p_data[p_buffer->i_data] &= ~p_buffer->i_mask;\
        }\
        p_buffer->i_mask >>= 1;         /*操作完一个字节第一位后，操作第二位*/\
        if( p_buffer->i_mask == 0 )     /*循环完一个字节的8位后，重新开始下一位*/\
        {\
            p_buffer->i_data++;\
            p_buffer->i_mask = 0x80;\
        }\
    }\
}
 
struct bits_buffer_s {
  unsigned char* p_data;
  unsigned char  i_mask;
  int i_size;
  int i_data;
};

uint32_t mpegCrc32(const uint8_t* data, uint32_t len);

#endif //Mpeg_H

