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