#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <string.h>
#include <iomanip>

#include "AacTrack.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

int avpriv_mpeg4audio_sample_rates[] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000, 7350,
    0, 0, 0
};

/// <summary>
/// AAC ADTS头部实体
/// </summary>
class AacADTSHeader
{
public:
	/// <summary>
	/// 同步头，12bit，总是0xFFF，all bits must be 1，代表着⼀个ADTS帧的开始。
	/// </summary>
	uint16_t synword = 0xFFF;						   
	/// <summary>
	/// 设置MPEG标识符，1bit，0标识MPEG-4，1标识MPEG-2。
	/// </summary>
	uint8_t id = 0; 								 
	/// <summary>
	/// 2bit always: '00'
	/// </summary>
	uint8_t layer = 0x00;		
	/// <summary>
	/// 误码校验，1bit，表示是否误码校验。标识是否进行误码校验。0表示有CRC校验，1表示没有CRC校验。为0时头部7bytes后面+2bytesCRC检验位，为1时只有7bytes
	/// </summary>
	/// <returns>误码校验</returns>
	uint8_t protection_absent = 1;     
	/// <summary>
	/// AAC级别，2bit，表示使用哪个级别的AAC，比如AAC LC=1。profile的值等于Audio Object Type的值减1
	/// </summary>
	uint8_t profile;		
	/// <summary>
	/// 采样率下标，4bit，表示使用的采样率下标
	/// 0: 96000 Hz
	/// 1: 88200 Hz
	/// 2 : 64000 Hz
	/// 3 : 48000 Hz
	/// 4 : 44100 Hz
	/// 5 : 32000 Hz
	/// 6 : 24000 Hz
	/// 7 : 22050 Hz
	/// 8 : 16000 Hz
	/// 9 : 12000 Hz
	/// 10 : 11025 Hz
	/// 11 : 8000 Hz
	/// 12 : 7350 Hz
	/// 13 : Reserved
	/// 14 : Reserved
	/// 15 : frequency is written explictly
	/// </summary>
	uint8_t sampling_frequency_index;			
	/// <summary>
	/// 私有位，1bit，编码时设置为0，解码时忽略
	/// </summary>
	/// <returns>私有位</returns>
	uint8_t private_bit = 0;	
	/// <summary>
	/// 声道数,3bit
	/// 0: Defined in AOT Specifc Config
	/// 1: 1 channel : front - center
	///	2 : 2 channels : front - left, front - right
	///	3 : 3 channels : front - center, front - left, front - right
	///	4 : 4 channels : front - center, front - left, front - right, back - center
	///	5 : 5 channels : front - center, front - left, front - right, back - left, back - right
	///	6 : 6 channels : front - center, front - left, front - right, back - left, back - right, LFE - channel
	///	7 : 8 channels : front - center, front - left, front - right, side - left, side - right, back - left, back - right, LFE - channel
	///	8 - 15 : Reserved
	/// </summary>
	uint8_t channel_configuration;	
	/// <summary>
	/// 1bit，编码时设置为0，解码时忽略。
	/// </summary>
	uint8_t orininal_copy = 0;	
	/// <summary>
	/// 1bit，编码时设置为0，解码时忽略。
	/// </summary>
	uint8_t home = 0;		
	/// <summary>
	/// 1bit，编码时设置为0，解码时忽略。
	/// </summary>
	uint8_t copyrigth_identification_bit = 0;		   
	/// <summary>
	/// 1bit，编码时设置为0，解码时忽略。
	/// </summary>
	uint8_t copyrigth_identification_stat = 0;		   
	/// <summary>
	///  aac帧长度，13bit，一个ADTS帧的⻓度包括ADTS头和AAC原始流
	/// </summary>
	uint16_t aac_frame_length = 0;					  
	/// <summary>
	///  11bit，0x7FF说明是码率可变的码流。
	/// </summary>
	uint16_t adts_bufferfullness = 0x7FF;			  
	/// <summary>
	///  2bit，表示ADTS帧中有value + 1个AAC原始帧，为0表示说ADTS帧中只有一个AAC数据块。
	/// </summary>
	uint8_t number_of_raw_data_blocks_in_frame = 0;   

public:
	FrameBuffer::Ptr toFrame();
};

FrameBuffer::Ptr AacADTSHeader::toFrame()
{
	if (aac_frame_length == 0) {
		return nullptr;
	}

	auto frame = make_shared<FrameBuffer>();
	frame->_startSize = 7;
	frame->_buffer.resize(7);
	auto data = frame->data();
	// data[0] = 0xFF;
	// data[1] = 0xF9;
	// data[2] = (((profile-1)<<6) + (sampling_frequency_index<<2) +(channel_configuration>>2));
	// data[3] = (((channel_configuration&3)<<6) + (aac_frame_length>>11));
	// data[4] = ((aac_frame_length&0x7FF) >> 3);
	// data[5] = (((aac_frame_length&7)<<5) + 0x1F);
	// data[6] = 0xFC;

	data[0] = (synword >> 4 & 0xFF); // 8bit
    data[1] = (synword << 4 & 0xF0); // 4 bit
    data[1] |= (id << 3 & 0x08); // 1 bit
    data[1] |= (layer << 1 & 0x06); // 2bit
    data[1] |= (protection_absent & 0x01); // 1 bit
    data[2] = (profile << 6 & 0xC0); // 2 bit
    data[2] |= (sampling_frequency_index << 2 & 0x3C); // 4bit
    data[2] |= (private_bit << 1 & 0x02); // 1 bit
    data[2] |= (channel_configuration >> 2 & 0x03); // 1 bit
    data[3] = (channel_configuration << 6 & 0xC0); // 2 bit
    data[3] |= (orininal_copy << 5 & 0x20); // 1 bit
    data[3] |= (home << 4 & 0x10); // 1 bit
    data[3] |= (copyrigth_identification_bit << 3 & 0x08); // 1 bit
    data[3] |= (copyrigth_identification_stat << 2 & 0x04); // 1 bit
    data[3] |= (aac_frame_length >> 11 & 0x03); // 2 bit
    data[4] = (aac_frame_length >> 3 & 0xFF); // 8 bit
    data[5] = (aac_frame_length << 5 & 0xE0); // 3 bit
    data[5] |= (adts_bufferfullness >> 6 & 0x1F); // 5 bit
    data[6] = (adts_bufferfullness << 2 & 0xFC); // 6 bit
    data[6] |= (number_of_raw_data_blocks_in_frame & 0x03); // 2 bit

	return frame;
}

string adtsToConfig(const char* data, int &samplerate, int& channel)
{
	// auto data = frame->data();
	if (!data) {
		return "";
	}

	if (!((uint8_t)data[0] == 0xFF && ((uint8_t)data[1] & 0xF0) == 0xF0)) {
        return "";
    }
	unsigned char profile = (data[2] & 0xC0) >> 6; // 2 bits
    if (profile == 3) {
        return "";
    }

    // Get and check the 'sampling_frequency_index':
    unsigned char sampling_frequency_index = (data[2] & 0x3C) >> 2; // 4 bits
    if (avpriv_mpeg4audio_sample_rates[sampling_frequency_index] == 0) {
        return "";
    }

	samplerate = avpriv_mpeg4audio_sample_rates[sampling_frequency_index];

    // Get and check the 'channel_configuration':
    unsigned char channel_configuration = ((data[2] & 0x01) << 2) | ((data[3] & 0xC0) >> 6); // 3 bits
	channel = channel_configuration;
    unsigned char audioSpecificConfig[2];
    unsigned char const audioObjectType = profile + 1;
    audioSpecificConfig[0] = (audioObjectType << 3) | (sampling_frequency_index >> 1);
    audioSpecificConfig[1] = (sampling_frequency_index << 7) | (channel_configuration << 3);
    return string((char *)audioSpecificConfig,2);
}

AacADTSHeader configToAdts(const string& config, int length/*aac frame length*/)
{
	AacADTSHeader adts;
	if (config.size() < 2) {
		logError << "invalid aac config:" << config;
		return adts;
	}

	uint8_t cfg1 = config[0];
    uint8_t cfg2 = config[1];

    int audioObjectType;
    int sampling_frequency_index;
    int channel_configuration;

    audioObjectType = cfg1 >> 3;
    sampling_frequency_index = ((cfg1 & 0x07) << 1) | (cfg2 >> 7);
    channel_configuration = (cfg2 & 0x7F) >> 3;

    adts.profile = audioObjectType - 1;
    adts.sampling_frequency_index = sampling_frequency_index;
    adts.channel_configuration = channel_configuration;
	adts.aac_frame_length = length;

	return adts;
}

void getSampleFromConfig(const string& config, int& samplerate, int& channel)
{
	if (config.size() < 2) {
		logError << "invalid aac config:" << config;
		return ;
	}

	uint8_t cfg1 = config[0];
    uint8_t cfg2 = config[1];

    int sampling_frequency_index = ((cfg1 & 0x07) << 1) | (cfg2 >> 7);

	samplerate = avpriv_mpeg4audio_sample_rates[sampling_frequency_index];
	channel = (cfg2 & 0x7F) >> 3;
}

int match_by_switch(int sampleRate) {
	static unordered_map<int, int> mapSampleRate = {{96000, 0}, {88200, 1}, {64000, 2}, {48000, 3}, 
				{44100, 4}, {32000, 5}, {24000, 6}, {22050, 7}, {16000, 8}, {12000, 9}, {11025, 10}, 
				{8000, 11}, {0, 12}};

	auto iter = mapSampleRate.find(sampleRate);
    if (iter == mapSampleRate.end()) {
		return -1;
	}

	return iter->second;
}

string makeAacConfig(int profile, int channel, int sampleRate)
{
    unsigned char sampling_frequency_index = match_by_switch(sampleRate);
    unsigned char channel_configuration = channel;
    unsigned char audioSpecificConfig[2];
    unsigned char const audioObjectType = profile;

    audioSpecificConfig[0] = (audioObjectType << 3) | (sampling_frequency_index >> 1);
    audioSpecificConfig[1] = (sampling_frequency_index << 7) | (channel_configuration << 3);
    return string((char *)audioSpecificConfig,2);
}

static StreamBuffer::Ptr getAacConfig()
{
	auto frame = make_shared<StreamBuffer>(4 + 1);
	int profile = 1;
    unsigned char sampling_frequency_index = 0xb;
    unsigned char channel_configuration = 1;
    char* audioSpecificConfig = frame->data();
    unsigned char const audioObjectType = profile;

	int audioType = 10;
	int flvSampleRate = 0;
	uint8_t flvStereoOrMono = (channel_configuration > 1);
	uint8_t flvSampleBit = 0;

	int audoFlag = (audioType << 4) | (flvSampleRate << 2) | (flvSampleBit << 1) | flvStereoOrMono;

	audioSpecificConfig[0] = audoFlag;
	audioSpecificConfig[1] = 0;
    audioSpecificConfig[2] = (audioObjectType << 3) | (sampling_frequency_index >> 1);
    audioSpecificConfig[3] = (sampling_frequency_index << 7) | (channel_configuration << 3);
    return frame;
}


AacTrack::AacTrack()
{

}

string AacTrack::getSdp()
{
	stringstream ss;
	ss << "m=audio 0 RTP/AVP " << payloadType_ << "\r\n"
	// ss << "b=AS:" << bitrate << "\r\n";
	   << "a=rtpmap:" << payloadType_ << " MPEG4-GENERIC/" << samplerate_ << "/" << channel_ << "\r\n";

	ss << "a=fmtp:" << payloadType_ << " streamtype=5;profile-level-id=1;mode=AAC-hbr;"
	   << "sizelength=13;indexlength=3;indexdeltalength=3;config=";
	
	for(auto &ch : _aacConfig){
		// snprintf(buf, sizeof(buf), "%02X", (uint8_t)ch);
		// configStr.append(buf);
		ss << std::hex << setw(2) << setfill('0') << (int)ch;
	}

	ss << "\r\na=control:trackID=" << index_ << "\r\n";

	return ss.str();
}

string AacTrack::getAacInfo()
{
    return _aacConfig;
}

StreamBuffer::Ptr AacTrack::getMuteConfig()
{
    static StreamBuffer::Ptr muteConfig = getAacConfig();

	return muteConfig;
}

void AacTrack::setAacInfo(const string& aacConfig)
{
	_aacConfig = aacConfig;
	getSampleFromConfig(_aacConfig, samplerate_, channel_);
}

void AacTrack::setAacInfo(int profile, int channel, int sampleRate)
{
	_aacConfig = makeAacConfig(profile, channel, sampleRate);
}

void AacTrack::setAacInfoByAdts(const char* data, int len)
{
	if (len < 7) {
		return ;
	}
	_aacConfig = adtsToConfig(data, samplerate_, channel_);
	// getSampleFromConfig(_aacConfig, samplerate_, channel_);
}

string AacTrack::getAdtsHeader(int frameSize)
{
	if (_aacConfig.empty()) {
		logError << "aac config is empty";
		return "";
	}
	
	auto adts = configToAdts(_aacConfig, frameSize + 7);
	auto frame = adts.toFrame();
	if (!frame) {
		return "";
	}

	return string(frame->data(), frame->size());
}