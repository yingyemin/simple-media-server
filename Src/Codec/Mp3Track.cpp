#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <string.h>
#include <iomanip>

#include "Mp3Track.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

enum Mp3Version
{
    MPEG2_5,
    Unknown_Version,
    MPEG2,
    MPEG1
};

enum Mp3Layer
{
    Unknown_Layer,
    Layer3,
    Layer2,
    Layer1
};

enum Mp3ChannelMode
{
    Stereo,
    JointStereo,
    DualChannel,
    Mono
};

// version samplerateIndex
uint16_t sampleRateTable[4][4] = {
    {11025, 12000, 8000, 0},
    {0, 0, 0, 0},
    {22050, 24000, 16000, 0},
    {44100, 48000, 32000, 0}
};

// version layer
// 一帧的采样数，通过码率，采样率和一帧的采样数计算一帧的大小
// Size = ((每帧采样数/8*比特率)/采样频率)+填充
uint16_t sampleNumPerFrame[4][4] = {
    {0, 384, 384, 384},
    {0, 0, 0, 0},
    {0, 1152, 1152, 1152},
    {0, 1152, 576, 576}
};

// index version layer
uint16_t bitrateTable[16][4][4] = {
    {
        {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
    },
    {
        {0, 0, 0, 0}, {0, 8, 32, 32}, {0, 8, 32, 32}, {0, 32, 32, 32},
    },
    {
        {0, 0, 0, 0}, {0, 16, 48, 64}, {0, 16, 48, 64}, {0, 40, 48, 64},
    },
    {
        {0, 0, 0, 0}, {0, 24, 56, 96}, {0, 24, 56, 96}, {0, 48, 54, 96},
    },
    {
        {0, 0, 0, 0}, {0, 32, 64, 128}, {0, 32, 64, 128}, {0, 56, 64, 128},
    },
    {
        {0, 0, 0, 0}, {0, 64, 80, 160}, {0, 64, 80, 160}, {0, 64, 80, 160},
    },
    {
        {0, 0, 0, 0}, {0, 80, 96, 192}, {0, 80, 96, 192}, {0, 80, 96, 192},
    },
    {
        {0, 0, 0, 0}, {0, 56, 112, 224}, {0, 56, 112, 224}, {0, 96, 112, 224},
    },
    {
        {0, 0, 0, 0}, {0, 64, 128, 256}, {0, 64, 128, 256}, {0, 112, 128, 256},
    },
    {
        {0, 0, 0, 0}, {0, 128, 160, 288}, {0, 128, 160, 288}, {0, 128, 160, 288},
    },
    {
        {0, 0, 0, 0}, {0, 160, 192, 320}, {0, 160, 192, 320}, {0, 160, 192, 320},
    },
    {
        {0, 0, 0, 0}, {0, 112, 224, 352}, {0, 112, 224, 352}, {0, 192, 224, 352},
    },
    {
        {0, 0, 0, 0}, {0, 128, 256, 384}, {0, 128, 256, 384}, {0, 224, 256, 384},
    },
    {
        {0, 0, 0, 0}, {0, 256, 320, 416}, {0, 256, 320, 416}, {0, 256, 320, 416},
    },
    {
        {0, 0, 0, 0}, {0, 320, 384, 448}, {0, 320, 384, 448}, {0, 320, 384, 448},
    },
    {
        {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
    },
};

Mp3Track::Mp3Track()
{
	payloadType_ = PayloadType_Mp3;
}

string Mp3Track::getSdp()
{
	stringstream ss;
	ss << "m=audio 0 RTP/AVP " << payloadType_ << "\r\n"
	   << "a=rtpmap:" << payloadType_ << " MPA" << "\r\n"
	   << "\r\na=control:trackID=" << index_ << "\r\n";

	return ss.str();
}

void Mp3Track::onFrame(const FrameBuffer::Ptr& frame)
{
    if (_hasReady) {
        return ;
    }

    if (!frame || frame->size() < 4) {
        return ;
    }

    uint8_t* payload = (uint8_t*)frame->data();
    int size = frame->size();
    int dataIndex = 0;

    if (payload[0] == 'I' && payload[1] == 'D' && payload[2] == '3') {
        // 参考类ID3v2
        dataIndex = readUint32BE((char*)payload + 6);
    }

    if (size < dataIndex + 4) {
        return ;
    }

    payload += dataIndex;

    // mp3帧头，参考MP3FrameHeader
    int sync1 = payload[0] << 3 | payload[1] >> 5;
    int version = payload[1] >> 3 & 0x03;
    int layer = payload[1] >> 1 & 0x03;
    int crcFlag = payload[1] & 0x01;

    if (sync1 != 0x7FF) {
        logError << "sync1 != 0x7FF";
        return ;
    }

    if (version == 0x01) {
        logError << "version != 0x01";
        return ;
    }

    if (layer == 0x00) {
        logError << "layer != 0x00";
        return ;
    }

    int bitrateIndex = payload[2] >> 4 & 0x0F;
    int sampleRateIndex = payload[2] >> 2 & 0x03;
    int padding = payload[2] >> 1 & 0x01;
    int reserved = payload[2] & 0x01;

    int channelMode = payload[3] >> 6 & 0x03;
    int modeExtension = payload[3] >> 4 & 0x03;
    int copyright = payload[3] >> 3 & 0x01;
    int original = payload[3] >> 2 & 0x01;
    int emphasis = payload[3] & 0x03;

    if (channelMode == Mp3ChannelMode::Mono) {
        channel_ = 1;
    } else {
        channel_ = 2;
    }

    samplerate_ = sampleRateTable[version][sampleRateIndex];
    bitPerSample_ = 16;
    timebase_ = 90000;

    int bitrate = bitrateTable[bitrateIndex][version][layer];

    logInfo << "channel is: " << channel_
            << ", samplerate_: " << samplerate_
            << ", bitrate: " << bitrate << " kbps";
    
	_hasReady = true;
}

// rtsp中，mp3的时间戳不是根据采样率才来算的，而是基于90000
Mp3Track::Ptr Mp3Track::createTrack(int index, int payloadType, int samplerate)
{
    auto trackInfo = make_shared<Mp3Track>();
    trackInfo->index_ = index;
    trackInfo->codec_ = "mp3";
    trackInfo->payloadType_ = payloadType;
    trackInfo->trackType_ = "audio";
    trackInfo->samplerate_ = samplerate;
    trackInfo->timebase_ = 90000;
    trackInfo->bitPerSample_ = 16;
    trackInfo->channel_ = 2;

    return trackInfo;
}

void Mp3Track::registerTrackInfo()
{
	TrackInfo::registerTrackInfo("mp3", [](int index, int payloadType, int samplerate){
		auto trackInfo = make_shared<Mp3Track>();
		trackInfo->index_ = AudioTrackType;
		trackInfo->codec_ = "mp3";
		trackInfo->payloadType_ = 14;
		trackInfo->trackType_ = "audio";
		trackInfo->samplerate_ = 44100;
        trackInfo->timebase_ = 90000;
        trackInfo->bitPerSample_ = 16;
        trackInfo->channel_ = 2;

		return trackInfo;
	});
}
