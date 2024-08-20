#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <cmath>

#include "H264Track.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/Base64.h"
#include "H264Nal.h"

using namespace std;

H264Track::H264Track()
{

}

string H264Track::getSdp()
{
    stringstream ss;
    ss << "m=video 0 RTP/AVP " << payloadType_ << "\r\n"
    // ss << "b=AS:" << bitrate << "\r\n";
       << "a=rtpmap:" << payloadType_ << " H264/" << samplerate_ << "\r\n"
       << "a=fmtp:" << payloadType_ << " packetization-mode=1";

    if (_sps && _pps) {
        char strTemp[100];
        uint32_t profile_level_id = 0;
        int length = _sps->size() - _sps->startSize();
        auto sps = string(_sps->data() + _sps->startSize(), _sps->size() - _sps->startSize());
        auto pps = string(_pps->data() + _pps->startSize(), _pps->size() - _pps->startSize());
        if (length >= 4) { // sanity check
            profile_level_id = (uint8_t(sps[1]) << 16) |
                                (uint8_t(sps[2]) << 8)  |
                                (uint8_t(sps[3])); // profile_idc|constraint_setN_flag|level_idc
        }
        memset(strTemp, 0, 100);
        sprintf(strTemp, "%06X", profile_level_id);
        ss << "; profile-level-id=" << strTemp;
        ss << "; sprop-parameter-sets=";
        ss << Base64::encode(sps) << ",";
        ss << Base64::encode(pps) << "\r\n";
    } else {
        ss << "\r\n";
    }
    ss << "a=control:trackID=" << index_ << "\r\n";

    return ss.str();
}

void H264Track::getWidthAndHeight(int& width, int& height, int& fps)
{
    if (!_width) {
        width = _width;
        height = _height;
        fps = samplerate_;

        return ;
    }
    auto sps = _sps->data() + _sps->startSize();
    auto size = _sps->size() - _sps->startSize();
    h264_decode_sps((unsigned char*)sps, size, _width, _height, samplerate_);

    width = _width;
    height = _height;
    fps = samplerate_;
}

string H264Track::getConfig()
{
    if (!_sps || !_pps) {
        return "";
    }
    
    string config;

    auto spsSize = _sps->startSize();
    auto ppsSize = _pps->startSize();

    auto spsBuffer = _sps->_buffer;
    int spsLen = _sps->size() - spsSize;
    auto ppsBuffer = _pps->_buffer;
    int ppsLen = _pps->size() - ppsSize;

    config.resize(11 + spsLen + ppsLen);
    auto data = (char*)config.data();

    // *data++ = 0x17; //key frame, AVC
    // *data++ = 0x00; //avc sequence header
    // *data++ = 0x00; //composit time 
    // *data++ = 0x00; //composit time
    // *data++ = 0x00; //composit time
    *data++ = 0x01;   //configurationversion
    *data++ = spsBuffer[spsSize + 1]; //avcprofileindication
    *data++ = spsBuffer[spsSize + 2]; //profilecompatibilty
    *data++ = spsBuffer[spsSize + 3]; //avclevelindication
    *data++ = 0xff;   //reserved + lengthsizeminusone
    *data++ = 0xe1;   //num of sps
    *data++ = (uint8_t)(spsLen >> 8); //sequence parameter set length high 8 bits
    *data++ = (uint8_t)(spsLen); //sequence parameter set  length low 8 bits
    // data length 13
    memcpy(data, spsBuffer.data() + spsSize, spsLen); //H264 sequence parameter set
    data += spsLen; // 13 + spsLen

    *data++ = 0x01; //num of pps
    *data++ = (uint8_t)(ppsLen >> 8); //picture parameter set length high 8 bits
    *data++ = (uint8_t)(ppsLen); //picture parameter set length low 8 bits
    // 16 + spsLen + ppsLen
    memcpy(data, ppsBuffer.data() + ppsSize, ppsLen); //H264 picture parameter set

    return config;
}