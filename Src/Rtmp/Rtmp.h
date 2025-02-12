#ifndef _RTMP_DEFINE_H_
#define _RTMP_DEFINE_H_

#include <string>

static const int RTMP_VERSION           = 0x3;

static const int RTMP_SET_CHUNK_SIZE    = 0x1; /* 设置块大小 */
static const int RTMP_AOBRT_MESSAGE     = 0X2; /* 终止消息 */
static const int RTMP_ACK               = 0x3; /* 确认 */
static const int RTMP_USER_EVENT        = 0x4; /* 用户控制消息 */
static const int RTMP_ACK_SIZE          = 0x5; /* 窗口大小确认 */
static const int RTMP_BANDWIDTH_SIZE    = 0x6; /* 设置对端带宽 */
static const int RTMP_AUDIO             = 0x08;
static const int RTMP_VIDEO             = 0x09;
static const int RTMP_FLEX_MESSAGE      = 0x11; //amf3
static const int RTMP_NOTIFY            = 0x12;
static const int RTMP_INVOKE            = 0x14; //amf0
static const int RTMP_FLASH_VIDEO       = 0x16;

static const int RTMP_CHUNK_TYPE_0      = 0; // 11
static const int RTMP_CHUNK_TYPE_1      = 1; // 7
static const int RTMP_CHUNK_TYPE_2      = 2; // 3
static const int RTMP_CHUNK_TYPE_3      = 3; // 0

static const int RTMP_CHUNK_CONTROL_ID  = 2;
static const int RTMP_CHUNK_INVOKE_ID   = 3;
static const int RTMP_CHUNK_AUDIO_ID    = 4;
static const int RTMP_CHUNK_VIDEO_ID    = 5;
static const int RTMP_CHUNK_DATA_ID     = 6;

static const int RTMP_CODEC_ID_H264     = 7;
static const int RTMP_CODEC_ID_H265     = 12;
static const int RTMP_CODEC_ID_AV1     = 13;
static const int RTMP_CODEC_ID_AVS3     = 14;
static const int RTMP_CODEC_ID_H266     = 15;
static const int RTMP_CODEC_ID_VP9     = 16;

static const int RTMP_CODEC_ID_MP3      = 2;
static const int RTMP_CODEC_ID_G711A    = 7;
static const int RTMP_CODEC_ID_G711U    = 8;
static const int RTMP_CODEC_ID_AAC      = 10;
static const int RTMP_CODEC_ID_OPUS      = 13;

static const int RTMP_StreamID_CONTROL  = 0;
static const int RTMP_StreamID_MEDIA    = 0;

static const int RTMP_AVC_SEQUENCE_HEADER = 0x18;
static const int RTMP_AAC_SEQUENCE_HEADER = 0x19;

std::string getCodecNameById(int trackType, int codeId);

#endif