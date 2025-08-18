#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <cmath>
#include<string.h>

#include "H265Track.h"
#include "H265Frame.h"
#include "NalBitstream.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/Base64.h"

using namespace std;
 
struct vc_params_t
{
	int width, height, fps;
	int profile, level;
	int nal_length_size;
	uint32_t PicSizeInCtbsY;
	void clear()
	{
		memset(this, 0, sizeof(*this));
	}
};

struct PpsInfo
{
	uint8_t dependent_slice_segments_enabled_flag;
	uint8_t num_extra_slice_header_bits;
};
 
bool  ParseSequenceParameterSet(uint8* data, int size, vc_params_t& params)
{
	if (!data || size < 20)
	{
		return false;
	}
	NALBitstream bs(data, size);
 
	// seq_parameter_set_rbsp()
	bs.GetWord(4);// sps_video_parameter_set_id
	int sps_max_sub_layers_minus1 = bs.GetWord(3); // "The value of sps_max_sub_layers_minus1 shall be in the range of 0 to 6, inclusive."
	if (sps_max_sub_layers_minus1 > 6)
	{
		return false;
	}
	bs.GetWord(1);// sps_temporal_id_nesting_flag
	// profile_tier_level( sps_max_sub_layers_minus1 )
	{
		bs.GetWord(2);// general_profile_space
		bs.GetWord(1);// general_tier_flag
		params.profile = bs.GetWord(5);// general_profile_idc
		bs.GetWord(32);// general_profile_compatibility_flag[32]
		bs.GetWord(1);// general_progressive_source_flag
		bs.GetWord(1);// general_interlaced_source_flag
		bs.GetWord(1);// general_non_packed_constraint_flag
		bs.GetWord(1);// general_frame_only_constraint_flag
		bs.GetWord(44);// general_reserved_zero_44bits
		params.level = bs.GetWord(8);// general_level_idc
		uint8 sub_layer_profile_present_flag[6] = { 0 };
		uint8 sub_layer_level_present_flag[6] = { 0 };
		for (int i = 0; i < sps_max_sub_layers_minus1; i++) {
			sub_layer_profile_present_flag[i] = bs.GetWord(1);
			sub_layer_level_present_flag[i] = bs.GetWord(1);
		}
 
		if (sps_max_sub_layers_minus1 > 0) {
			for (int i = sps_max_sub_layers_minus1; i < 8; i++) {
				uint8 reserved_zero_2bits = bs.GetWord(2);
			}
		}
 
		for (int i = 0; i < sps_max_sub_layers_minus1; i++) {
			if (sub_layer_profile_present_flag[i]) {
				bs.GetWord(2);// sub_layer_profile_space[i]
				bs.GetWord(1);// sub_layer_tier_flag[i]
				bs.GetWord(5);// sub_layer_profile_idc[i]
				bs.GetWord(32);// sub_layer_profile_compatibility_flag[i][32]
				bs.GetWord(1);// sub_layer_progressive_source_flag[i]
				bs.GetWord(1);// sub_layer_interlaced_source_flag[i]
				bs.GetWord(1);// sub_layer_non_packed_constraint_flag[i]
				bs.GetWord(1);// sub_layer_frame_only_constraint_flag[i]
				bs.GetWord(44);// sub_layer_reserved_zero_44bits[i]
			}
 
			if (sub_layer_level_present_flag[i]) {
				bs.GetWord(8);// sub_layer_level_idc[i]
			}
		}
	}
 
	uint32 sps_seq_parameter_set_id = bs.GetUE(); // "The  value  of sps_seq_parameter_set_id shall be in the range of 0 to 15, inclusive."
	if (sps_seq_parameter_set_id > 15) {
		return false;
	}
 
	uint32 chroma_format_idc = bs.GetUE(); // "The value of chroma_format_idc shall be in the range of 0 to 3, inclusive."
	if (sps_seq_parameter_set_id > 3) {
		return false;
	}
 
	if (chroma_format_idc == 3) {
		bs.GetWord(1);// separate_colour_plane_flag
	}
 
	params.width = bs.GetUE(); // pic_width_in_luma_samples
	params.height = bs.GetUE(); // pic_height_in_luma_samples
 
	if (bs.GetWord(1)) {// conformance_window_flag
		bs.GetUE();  // conf_win_left_offset
		bs.GetUE();  // conf_win_right_offset
		bs.GetUE();  // conf_win_top_offset
		bs.GetUE();  // conf_win_bottom_offset
	}
 
	uint32 bit_depth_luma_minus8 = bs.GetUE();
	uint32 bit_depth_chroma_minus8 = bs.GetUE();
 
	if (bit_depth_luma_minus8 != bit_depth_chroma_minus8) {
		return false;
	}

	bs.GetUE(); //log2_max_pic_order_cnt_lsb_minus4
	uint8_t sps_sub_layer_ordering_info_present_flag = bs.GetWord(1); //sps_sub_layer_ordering_info_present_flag

	for (int i = sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers_minus1; i <= sps_max_sub_layers_minus1; ++i) {
		bs.GetUE(); //sps_max_dec_pic_buffering_minus1
		bs.GetUE(); //sps_max_num_reorder_pics
		bs.GetUE(); //sps_max_latency_increase_plus1
	}

	uint32_t log2_min_luma_coding_block_size_minus3 = bs.GetUE();
	uint32_t log2_diff_max_min_luma_coding_block_size = bs.GetUE();

	uint32_t MinCbLog2SizeY = log2_min_luma_coding_block_size_minus3 + 3;
	uint32_t CtbLog2SizeY = MinCbLog2SizeY + log2_diff_max_min_luma_coding_block_size;
	uint32_t CtbSizeY = 1 << CtbLog2SizeY;
	uint32_t PicWidthInCtbsY = std::ceil(params.width / CtbSizeY);
	uint32_t PicHeightInCtbsY = std::ceil(params.height / CtbSizeY);

	params.PicSizeInCtbsY = PicWidthInCtbsY * PicHeightInCtbsY;
 
	return true;
}

bool  ParsePps(uint8* data, int size, PpsInfo& pps)
{
	if (!data || size <= 0) {
		return false;
	}

	NALBitstream bs(data, size);
	bs.GetUE(); //pps_pic_parameter_set_id
	bs.GetUE(); //pps_seq_parameter_set_id
	pps.dependent_slice_segments_enabled_flag = bs.GetWord(1);
	bs.GetWord(1); //output_flag_present_flag
	pps.num_extra_slice_header_bits = bs.GetWord(3);

	return true;
}

static void de_emulation_prevention(unsigned char* buf,unsigned int* buf_size)
{  
	if (!buf) {
		return ;
	}
    int i=0,j=0;  
    unsigned char* tmp_ptr=NULL;  
    unsigned int tmp_buf_size=0;  
    int val=0;  
  
    tmp_ptr=buf;  
    tmp_buf_size=*buf_size;  
    for(i=0;i<(tmp_buf_size-2);i++)  
    {  
        //check for 0x000003  
        val=(tmp_ptr[i]^0x00) +(tmp_ptr[i+1]^0x00)+(tmp_ptr[i+2]^0x03);  
        if(val==0)  
        {  
            //kick out 0x03  
            for(j=i+2;j<tmp_buf_size-1;j++)  
                tmp_ptr[j]=tmp_ptr[j+1];  
  
            //and so we should devrease bufsize  
            (*buf_size)--;  
        }  
    }  
}  

H265Track::H265Track()
{

}

string H265Track::getSdp()
{
    stringstream ss;
    ss << "m=video 0 RTP/AVP " << payloadType_ << "\r\n"
    //    << "b=AS:" << bitrate << "\r\n"
       << "a=rtpmap:" << payloadType_ << " H265/" << samplerate_ << "\r\n"
       << "a=fmtp:" << payloadType_;
	if (_vps && _sps && _pps) {
		auto vps = string(_vps->data() + _vps->startSize(), _vps->size() - _vps->startSize());
		auto sps = string(_sps->data() + _sps->startSize(), _sps->size() - _sps->startSize());
		auto pps = string(_pps->data() + _pps->startSize(), _pps->size() - _pps->startSize());

		ss << " sprop-vps=" << Base64::encode(vps) << "; "
		   << "sprop-sps=" << Base64::encode(sps) << "; "
		   << "sprop-pps=" << Base64::encode(pps) << "\r\n";
	} else {
		ss << "\r\n";
	}

	ss << "a=control:trackID=" << index_ << "\r\n";

	return ss.str();
}

void H265Track::getWidthAndHeight(int& width, int& height, int& fps)
{
    if (_width && _height) {
        width = _width;
        height = _height;

		return ;
    }
	
	if (!_sps || !_vps || !_pps) {
		logError << "no sps/pps/vps";
		return ;
	}

    vc_params_t params = {0};
    ParseSequenceParameterSet((uint8_t*)(_sps->data() + _sps->startSize()), _sps->size() - _sps->startSize(), params);
    _width = params.width;
    _height = params.height;
    width = _width;
    height = _height;
}

string H265Track::getConfig()
{
    if (!_vps || !_sps || !_pps) {
        return "";
    }

    string config;

    auto spsSize = _sps->startSize();
    auto vpsSize = _vps->startSize();
    auto ppsSize = _pps->startSize();

    auto vpsBuffer = _vps->_buffer;
    int vpsLen = _vps->size() - vpsSize;
    auto spsBuffer = _sps->_buffer;
    int spsLen = _sps->size() - spsSize;
    auto ppsBuffer = _pps->_buffer;
    int ppsLen = _pps->size() - ppsSize;

    config.resize(38  + vpsLen + spsLen + ppsLen);
    auto data = (char*)config.data();

    // *data++ = 0x1c; //key frame, AVC
    // *data++ = 0x00; //avc sequence header
    // *data++ = 0x00; //composit time 
    // *data++ = 0x00; //composit time
    // *data++ = 0x00; //composit time
    *data++ = 0x01;   //configurationversion
    // 6 byte

    *data++ = spsBuffer[spsSize + 5];  //general_profile_space(2), general_tier_flag(1), general_profile_idc(5)

    *data++ = spsBuffer[spsSize + 6]; //general_profile_compatibility_flags
    *data++ = spsBuffer[spsSize + 7]; 
    *data++ = spsBuffer[spsSize + 8]; 
    *data++ = spsBuffer[spsSize + 9]; 

    *data++ = spsBuffer[spsSize + 10]; //general_constraint_indicator_flags
    *data++ = spsBuffer[spsSize + 11]; 
    *data++ = spsBuffer[spsSize + 12]; 
    *data++ = spsBuffer[spsSize + 13]; 
    *data++ = spsBuffer[spsSize + 14]; 
    *data++ = spsBuffer[spsSize + 15]; 

    *data++ = spsBuffer[spsSize + 16]; // general_level_idc

    *data++ = spsBuffer[spsSize + 17]; // reserved(4),min_spatial_segmentation_idc(12)
    *data++ = spsBuffer[spsSize + 18]; 

    *data++ = spsBuffer[spsSize + 19]; // reserved(6),parallelismType(2)

    *data++ = spsBuffer[spsSize + 20]; // reserved(6),chromaFormat(2)
    // 22 byte 

    // 下面几个0xff乱填的，貌似不影响播放
    *data++ = 0xf << 4 | 0xf; //// reserved(5),bitDepthLumaMinus8(3)

    *data++ = 0xff; // reserved(5),bitDepthChromaMinus8(3)

    *data++ = 0xff; //avgFrameRate
    *data++ = 0xff;

    *data++ = 0xff; //constantFrameRate(2),numTemporalLayers(3),temporalIdNested(1),lengthSizeMinusOne(2)

    // *data++ = 0xff;

    // *data++ = 0xff;
    // *data++ = 0xff;
    // *data++ = 0xff;
    // 默认vps，sps，pps 各一个
    *data++ = 3; //numOfArrays
    //28 byte

    *data++ = 1 << 7 | H265NalType::H265_VPS & 0x3f; //array_completeness(1),reserved(1),NAL_unit_type(6)

    // 一个vps
    *data++ = 0; //numNalus
    *data++ = 1;

    *data++ = (uint8_t)(vpsLen >> 8); //sequence parameter set length high 8 bits
    *data++ = (uint8_t)(vpsLen); //sequence parameter set  length low 8 bits

    memcpy(data, vpsBuffer.data() + vpsSize, vpsLen); //H264 sequence parameter set
    data += vpsLen; 
    // 37 + vpsLen

    *data++ = 1 << 7 | H265NalType::H265_SPS & 0x3f;

    // 一个sps
    *data++ = 0;
    *data++ = 1;

    *data++ = (uint8_t)(spsLen >> 8); //sequence parameter set length high 8 bits
    *data++ = (uint8_t)(spsLen); //sequence parameter set  length low 8 bits

    memcpy(data, spsBuffer.data() + spsSize, spsLen); //H264 sequence parameter set
    data += spsLen; 
    // 38 + vpsLen + spsLen

    *data++ = 1 << 7 | H265NalType::H265_PPS & 0x3f;

    // 一个pps
    *data++ = 0;
    *data++ = 1;

    *data++ = (uint8_t)(ppsLen >> 8); //sequence parameter set length high 8 bits
    *data++ = (uint8_t)(ppsLen); //sequence parameter set  length low 8 bits

    memcpy(data, ppsBuffer.data() + ppsSize, ppsLen); //H264 sequence parameter set
    data += ppsLen; 
    // 43  + vpsLen + spsLen + ppsLen

    return config;
}

void H265Track::setConfig(const string& config)
{
	_hvcc = config;
	if (config.size() < 23) {
		return ;
	}

	int numOfArr = config[22];
	int index = 23;
	logInfo << "numOfArr is : " << numOfArr;
	for (int i = 0; i < numOfArr; ++i) {
		int len =(config[index + 1] & 0xFF) << 8 | (config[index + 2] & 0xFF);
		index += 3;
		for (int j = 0; j < len; ++j) {
			int naluLen =(config[index] & 0xFF) << 8 | (config[index + 1] & 0xFF);
			index += 2;

			auto frame = make_shared<H265Frame>();
			frame->_startSize = 4;
			frame->_codec = "h265";
			frame->_index = index_;
			frame->_trackType = VideoTrackType;

			frame->_buffer.assign("\x00\x00\x00\x01", 4);
			frame->_pts = frame->_dts = 0;
			frame->_buffer.append((char*)config.data() + index, naluLen);
			index += naluLen;

			logInfo << "get a config frame: " << (int)frame->getNalType();
			if (frame->getNalType() == H265_VPS) {
				setVps(frame);
			} else if (frame->getNalType() == H265_SPS) {
				setSps(frame);
			} else if (frame->getNalType() == H265_PPS) {
				setPps(frame);
			}
		}
	}
}

bool H265Track::isBFrame(uint8* data, int size)
{
	if (!data || !_vps || !_sps || !_pps) {
		return false;
	}
	// de_emulation_prevention(data, (unsigned int *)&size);

	if (!_dependent_slice_segments_enabled_flag && !_num_extra_slice_header_bits) {
		PpsInfo ppsInfo;
		ParsePps((uint8*)_pps->data() + _pps->startSize(), _pps->size() - _pps->startSize(), ppsInfo);
		_dependent_slice_segments_enabled_flag = ppsInfo.dependent_slice_segments_enabled_flag;
		_num_extra_slice_header_bits = ppsInfo.num_extra_slice_header_bits;
	}
	if (!_width && !_height) {
		vc_params_t param;
		ParseSequenceParameterSet((uint8*)_sps->data() + _sps->startSize(), _sps->size() - _sps->startSize(), param);
		_width = param.width;
		_height = param.height;
		_PicSizeInCtbsY = param.PicSizeInCtbsY;
	}
	// 跳过 nalu header 两个字节
	NALBitstream bs(data + 2, size - 2);
	uint8_t dependent_slice_segment_flag = 0;
	uint8_t first_slice_segment_in_pic_flag = bs.GetWord(1); //first_slice_segment_in_pic_flag
	uint8_t nal_unit_type = ((uint8_t)(data[0]) >> 1) & 0x3f;
	if (nal_unit_type >= 16/*H265_BLA_W_LP*/ && nal_unit_type <= 23/*H265_RSV_IRAP_VCL23*/)
	{
		bs.GetWord(1); //no_output_of_prior_pics_flag)
	}
	bs.GetUE();//slice_pic_parameter_set_id
	if (!first_slice_segment_in_pic_flag) {
		if (_dependent_slice_segments_enabled_flag/*pps*/) {
			dependent_slice_segment_flag = bs.GetWord(1);
		}
		bs.GetWord((uint32_t)std::ceil(std::log2(_PicSizeInCtbsY/*sps*/))); //slice_segment_address
	}
	if (!dependent_slice_segment_flag) {
		for (uint32_t i=0; i<_num_extra_slice_header_bits/*pps*/; i++)
		{
			bs.GetWord(1);//slice_reserved_flag
		}
		uint32_t slice_type = bs.GetUE();
		// logInfo << "slice_type ========= " << (int)slice_type << ", nalu type: " << (int)nal_unit_type;
		if (slice_type == 0) {
			return true;
		}
	}

	return false;
}

H265Track::Ptr H265Track::createTrack(int index, int payloadType, int samplerate)
{
    auto trackInfo = make_shared<H265Track>();
    trackInfo->index_ = index;
    trackInfo->codec_ = "h265";
    trackInfo->payloadType_ = payloadType;
    trackInfo->trackType_ = "video";
    trackInfo->samplerate_ = samplerate;

    return trackInfo;
}

void H265Track::registerTrackInfo()
{
	TrackInfo::registerTrackInfo("h265", [](int index, int payloadType, int samplerate){
		auto trackInfo = make_shared<H265Track>();
		trackInfo->index_ = VideoTrackType;
		trackInfo->codec_ = "h265";
		trackInfo->payloadType_ = 96;
		trackInfo->trackType_ = "video";
		trackInfo->samplerate_ = 90000;

		return trackInfo;
	});
}

void H265Track::onFrame(const FrameBuffer::Ptr& frame)
{
	if (_sps && _pps && _vps) {
		_hasReady = true;
		// return ;
	}

	if (!frame || frame->size() < 5) {
		return ;
	}

	if (frame->getNalType() == H265_VPS) {
        setVps(frame);
    } else if (frame->getNalType() == H265_SPS) {
        setSps(frame);
    } else if (frame->getNalType() == H265_PPS) {
        setPps(frame);
    }
}