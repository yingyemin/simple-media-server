#include "Rtmp.h"

using namespace std;

string getCodecNameById(int trackType, int codeId)
{
    if (trackType == 0) { //video
        switch (codeId)
        {
        case RTMP_CODEC_ID_H264:
            return "h264";
            break;
        
        case RTMP_CODEC_ID_H265:
            return "h265";
            break;
        
        case RTMP_CODEC_ID_H266:
            return "h266";
            break;
        
        case RTMP_CODEC_ID_AV1:
            return "av1";
            break;
        
        case RTMP_CODEC_ID_VP9:
            return "vp9";
            break;
        
        default:
            return "";
            break;
        }
    } else if (trackType == 1) { //audio
        switch (codeId)
        {
        case RTMP_CODEC_ID_AAC:
            return "aac";
            break;
        
        case RTMP_CODEC_ID_G711A:
            return "g711a";
            break;
        
        case RTMP_CODEC_ID_G711U:
            return "g711u";
            break;
        
        case RTMP_CODEC_ID_MP3:
            return "mp3";
            break;
        
        default:
            return "";
            break;
        }
    }

    return "";
}