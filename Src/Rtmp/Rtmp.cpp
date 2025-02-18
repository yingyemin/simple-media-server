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
        
        case RTMP_CODEC_ID_OPUS:
            return "opus";
            break;
        
        default:
            return "";
            break;
        }
    }

    return "";
}

int getIdByCodecName(int trackType, const string& codeName)
{
    if (trackType == 0) {
        if (codeName == "h264") {
            return RTMP_CODEC_ID_H264;
        } else if (codeName == "h265") {
            return RTMP_CODEC_ID_H265;
        } else if (codeName == "h266") {
            return RTMP_CODEC_ID_H266;
        } else if (codeName == "av1") {
            return RTMP_CODEC_ID_AV1;
        } else if (codeName == "vp9") {
            return RTMP_CODEC_ID_VP9;
        }
    } else {
        if (codeName == "aac") {
            return RTMP_CODEC_ID_AAC;
        } else if (codeName == "g711a") {
            return RTMP_CODEC_ID_G711A;
        } else if (codeName == "g711u") {
            return RTMP_CODEC_ID_G711U;
        } else if (codeName == "opus") {
            return RTMP_CODEC_ID_OPUS;
        } else if (codeName == "mp3") {
            return RTMP_CODEC_ID_MP3;
        }
    }

    return 0;
}