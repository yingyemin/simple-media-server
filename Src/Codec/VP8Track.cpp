#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <cmath>
#include<string.h>

#include "VP8Track.h"
#include "VP8Frame.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/Base64.h"

using namespace std;

VP8Track::VP8Track()
{

}

string VP8Track::getSdp()
{
    stringstream ss;
    ss << "m=video 0 RTP/AVP " << payloadType_ << "\r\n"
    //    << "b=AS:" << bitrate << "\r\n"
       << "a=rtpmap:" << payloadType_ << " VP8/" << samplerate_ << "\r\n"
       << "a=control:trackID=" << index_ << "\r\n";

	return ss.str();
}

void VP8Track::getWidthAndHeight(int& width, int& height, int& fps)
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
string VP8Track::getConfig()
{
    string config;
    config.resize(8);

    char* data = (char*)config.data();

    data[0] = _profile;
    data[1] = _level;
    data[2] = (_bitDepth << 4);
    data[3] = 0;
    data[4] = 0;
    data[5] = 0;
    data[6] = 0;
    data[7] = 0;

    return config;
}

void VP8Track::setConfig(const string& config)
{
    _vp8Cfg = config;
    if (config.size() < 8) {
        return ;
    }

    _profile = config[0];
    _level = config[1];
    _bitDepth = config[2] >> 4;
}

bool VP8Track::isBFrame(unsigned char* data, int size)
{
	return false;
}

VP8Track::Ptr VP8Track::createTrack(int index, int payloadType, int samplerate)
{
    auto trackInfo = make_shared<VP8Track>();
    trackInfo->index_ = index;
    trackInfo->codec_ = "VP8";
    trackInfo->payloadType_ = payloadType;
    trackInfo->trackType_ = "video";
    trackInfo->samplerate_ = samplerate;

    return trackInfo;
}

void VP8Track::registerTrackInfo()
{
	TrackInfo::registerTrackInfo("VP8", [](int index, int payloadType, int samplerate){
		auto trackInfo = make_shared<VP8Track>();
		trackInfo->index_ = VideoTrackType;
		trackInfo->codec_ = "VP8";
		trackInfo->payloadType_ = 96;
		trackInfo->trackType_ = "video";
		trackInfo->samplerate_ = 90000;

		return trackInfo;
	});
}

void VP8Track::onFrame(const FrameBuffer::Ptr& frame)
{
	if (_hasReady) {
        return ;
    }

    auto vp8Frame = dynamic_pointer_cast<VP8Frame>(frame);
    
    uint32_t tag;
    const uint8_t* p;
    const static uint8_t startcode[] = { 0x9d, 0x01, 0x2a };

    if (vp8Frame->size() < 10)
        return ;

    p = (const uint8_t*)vp8Frame->data();

    // 9.1.  Uncompressed Data Chunk
    tag = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
    //key_frame = tag & 0x01;
    //version = (tag >> 1) & 0x07;
    //show_frame = (tag >> 4) & 0x1;
    //first_part_size = (tag >> 5) & 0x7FFFF;

    if (0 == (tag & 0x01) && startcode[0] == p[3] && startcode[1] == p[4] && startcode[2] == p[5]) {
        _hasReady = true; // not key frame
    }

    _width = ((uint16_t)(p[7] & 0x3F) << 8) | (uint16_t)(p[6]); // (2 bits Horizontal Scale << 14) | Width (14 bits)
    _height = ((uint16_t)(p[9] & 0x3F) << 8) | (uint16_t)(p[8]); // (2 bits Vertical Scale << 14) | Height (14 bits)

    _profile = (tag >> 1) & 0x03;
    _level = 31;
    _bitDepth = 8;
}