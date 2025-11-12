#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <cmath>
#include<string.h>

#include "VP9Track.h"
#include "VP9Frame.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "Util/Base64.h"

using namespace std;

VP9Track::VP9Track()
{

}

string VP9Track::getSdp()
{
    stringstream ss;
    ss << "m=video 0 RTP/AVP " << payloadType_ << "\r\n"
    //    << "b=AS:" << bitrate << "\r\n"
       << "a=rtpmap:" << payloadType_ << " VP9/" << samplerate_ << "\r\n"
       << "a=control:trackID=" << index_ << "\r\n";

	return ss.str();
}

void VP9Track::getWidthAndHeight(int& width, int& height, int& fps)
{
    width = _width;
    height = _height;
}

/*
aligned (8) class VPCodecConfigurationRecord {
    unsigned int (8)     profile;
    unsigned int (8)     level;
    unsigned int (4)     bitDepth;
    unsigned int (3)     chromaSubsampling;
    unsigned int (1)     videoFullRangeFlag;
    unsigned int (8)     colourPrimaries;
    unsigned int (8)     transferCharacteristics;
    unsigned int (8)     matrixCoefficients;
    unsigned int (16)    codecIntializationDataSize;
    unsigned int (8)[]   codecIntializationData;
}
*/
string VP9Track::getConfig()
{
    string config;
    config.resize(8 + _codec_intialization_data_size);

    char* data = (char*)config.data();

    data[0] = _profile;
    data[1] = _level;
    data[2] = (_bitDepth << 4) | ((_chroma_subsampling & 0x07) << 1) | (_video_full_range_flag & 0x01);
    data[3] = _colour_primaries;
    data[4] = _transfer_characteristics;
    data[5] = _matrix_coefficients;
    data[6] = (uint8_t)(_codec_intialization_data_size >> 8);
    data[7] = (uint8_t)_codec_intialization_data_size;

    if(_codec_intialization_data_size > 0)
        memcpy(data + 8, _codec_intialization_data.data(), _codec_intialization_data_size);

    return config;
}

void VP9Track::setConfig(const string& config)
{
    _VP9Cfg = config;
    if (config.size() < 8) {
        return ;
    }

    _profile = config[0];
    _level = config[1];
    _bitDepth = (config[2] >> 4) & 0x0F;
    _chroma_subsampling = (config[2] >> 1) & 0x07;
    _video_full_range_flag = config[2] & 0x01;
    _colour_primaries = config[3];
    _transfer_characteristics = config[4];
    _matrix_coefficients = config[5];
    _codec_intialization_data_size = (((uint16_t)config[6]) << 8) | config[7];
    if(0 != _codec_intialization_data_size) {
        logWarn << "invalid config";
    }
}

bool VP9Track::isBFrame(unsigned char* data, int size)
{
	return false;
}

VP9Track::Ptr VP9Track::createTrack(int index, int payloadType, int samplerate)
{
    auto trackInfo = make_shared<VP9Track>();
    trackInfo->index_ = index;
    trackInfo->codec_ = "vp9";
    trackInfo->payloadType_ = payloadType;
    trackInfo->trackType_ = "video";
    trackInfo->samplerate_ = samplerate;

    return trackInfo;
}

void VP9Track::registerTrackInfo()
{
	TrackInfo::registerTrackInfo("vp9", [](int index, int payloadType, int samplerate){
		auto trackInfo = make_shared<VP9Track>();
		trackInfo->index_ = VideoTrackType;
		trackInfo->codec_ = "vp9";
		trackInfo->payloadType_ = 96;
		trackInfo->trackType_ = "video";
		trackInfo->samplerate_ = 90000;

		return trackInfo;
	});
}

void VP9Track::onFrame(const FrameBuffer::Ptr& frame)
{
	if (_hasReady) {
        return ;
    }
    
    uint8_t* nalByte;
    size_t len = frame->size();

    if (frame->size() < 10)
        return ;

    nalByte = (uint8_t*)frame->data();

    // 好像只有关键帧的startcode才是这个
    // 非关键帧的startcode是 86 00 40 92
    const static uint8_t startcode[] = { 0x49, 0x83, 0x42 };

    if (len < 4 || nalByte[1] != startcode[0] || nalByte[2] != startcode[1] || nalByte[3] != startcode[2]) {
        return ;
    }

    VP9Bitstream bitsream(nalByte, len);
    bitsream.GetWord(2); // frame marker
    _profile = bitsream.GetBit() | bitsream.GetBit() << 1;
    if (_profile == 3) {
        bitsream.GetBit(); //shall be 0
    }

    int show_existing_frame = bitsream.GetBit();
    if (show_existing_frame) {
        bitsream.GetWord(3); // frame_to_show_map_idx
    }

    int frame_type = bitsream.GetBit();
    bitsream.GetBit(); //show_frame 
    bitsream.GetBit(); //error_resilient_mode 

    if(frame_type == VP9_KEYFRAME) {
        _hasReady = true;
        bitsream.GetWord(24); //frame_sync_code 3bytes

        //color_config()
        if (_profile >= 2) {
            int ten_or_twelve_bit = bitsream.GetBit();
            _bitDepth = ten_or_twelve_bit ? 12 : 10;
        } else {
            _bitDepth = 8;
        }

        uint8_t color_space = bitsream.GetWord(3);
        if (7 /*CS_RGB*/ != color_space) // 3-color_space
        {
            _video_full_range_flag = (uint8_t)bitsream.GetBit(); // color_range
            if (1 == _profile || 3 == _profile)
            {
                _chroma_subsampling = 3 - (uint8_t)bitsream.GetWord(2); // subsampling_x/subsampling_y
                bitsream.GetBit(); // reserved_zero
            }
        }
        else
        {
            if (1 == _profile || 3 == _profile)
                bitsream.GetBit(); // reserved_zero
        }
    
        // frame_size()
        _width = (int)bitsream.GetWord(16) + 1;
        _height = (int)bitsream.GetWord(16) + 1;
        // return bitsream.GetBit() ? -1 : 0;
    }
}