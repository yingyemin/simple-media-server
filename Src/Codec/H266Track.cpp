#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <cmath>
#include<string.h>

#include "H266Track.h"
#include "H266Frame.h"
#include "NalBitstream.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "Util/Base64.h"

using namespace std;

#ifdef __GNUC__
// GCC/Clang 环境：使用 __builtin_constant_p
#define IS_CONSTANT(x) __builtin_constant_p(x)
#else
// MSVC 环境：使用替代方案（见下文）
#define IS_CONSTANT(x) false  // 简单替代，或根据需求实现
#endif

class vvcParam
{
public:
    int width;
    int height;
};

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

static inline int av_ceil_log2(unsigned int x)
{
    if (x <= 1)
        return 0;
    int pos = 0;
    while (x > 1) {
        x >>= 1;
        pos++;
    }
    return pos;
}

void parseVvcSps(uint8* data, int size, vvcParam& params)
{
    if (!data || size < 20)
	{
		return;
	}

	NALBitstream bs(data, size);
    // nal header,tow bytes
    bs.GetBYTE();
    bs.GetBYTE();

    bs.GetWord(8); // sps_seq_parameter_set_id && sps_video_parameter_set_id
    int sps_max_sublayers_minus1 = bs.GetWord(3);
    bs.GetWord(2); // chroma_format_idc
    int sps_log2_ctu_size_minus5 = bs.GetWord(2); // sps_log2_ctu_size_minus5
    if (bs.GetWord(1)) { // sps_ptl_dpb_hrd_params_present_flag
        int ptl_present_flag = 1;
        // vvcc_parse_ptl
        if (ptl_present_flag) {
            int general_profile_idc = bs.GetWord(7);
            int general_tier_flag = bs.GetWord(1);
        }
        int general_level_idc = bs.GetWord(8);

        int ptl_frame_only_constraint_flag = bs.GetWord(1);
        int ptl_multilayer_enabled_flag = bs.GetWord(1);
        if (ptl_present_flag) {       // parse constraint info
            int num_bytes_constraint_info = bs.GetWord(1); // gci_present_flag
            if (num_bytes_constraint_info) {
                int gci_num_reserved_bits, j;
                for (j = 0; j < 8; j++)
                    bs.GetWord(8); // general_constraint_info
                bs.GetWord(7); // general_constraint_info

                gci_num_reserved_bits = bs.GetWord(8);
                bs.GetWord(gci_num_reserved_bits);
            }
            bs.Align();
        }

        uint8_t ptl_sublayer_level_present_flag[8];
        for (int i = sps_max_sublayers_minus1 - 1; i >= 0; i--)
            ptl_sublayer_level_present_flag[i] = bs.GetWord(1); // ptl_sublayer_level_present_flag

        bs.Align();

        for (int i = sps_max_sublayers_minus1 - 1; i >= 0; i--) {
            if (ptl_sublayer_level_present_flag[i])
                bs.GetWord(8); //sublayer_level_idc
        }

        if (ptl_present_flag) {
            int ptl_num_sub_profiles = bs.GetWord(8);
            if (ptl_num_sub_profiles) {
                for (int i = 0; i < ptl_num_sub_profiles; i++)
                    bs.GetWord(32); //general_sub_profile_idc
            }
        }
    }

    bs.GetWord(1); // sps_gdr_enabled_flag
    if (bs.GetWord(1)) { // sps_ref_pic_resampling_enabled_flag
        bs.GetWord(1); // sps_res_change_in_clvs_allowed_flag
    }

    int sps_pic_width_max_in_luma_samples = bs.GetUE();
    params.width = sps_pic_width_max_in_luma_samples;
    int sps_pic_height_max_in_luma_samples = bs.GetUE();
    params.height = sps_pic_height_max_in_luma_samples;

    if (bs.GetBit()) {
        bs.GetUE(); // sps_conf_win_left_offset
        bs.GetUE(); // sps_conf_win_right_offset
        bs.GetUE(); // sps_conf_win_top_offset
        bs.GetUE(); // sps_conf_win_bottom_offset
    }

    if (bs.GetBit()) {        // sps_subpic_info_present_flag
        const unsigned int sps_num_subpics_minus1 = bs.GetUE();
        const int ctb_log2_size_y = sps_log2_ctu_size_minus5 + 5;
        const int ctb_size_y      = 1 << ctb_log2_size_y;
        const int tmp_width_val   = (!IS_CONSTANT(ctb_log2_size_y) ? -((-(sps_pic_width_max_in_luma_samples)) >> (ctb_log2_size_y)) : ((sps_pic_width_max_in_luma_samples) + (1<<(ctb_log2_size_y)) - 1) >> (ctb_log2_size_y));
        const int tmp_height_val  = (!IS_CONSTANT(ctb_log2_size_y) ? -((-(sps_pic_height_max_in_luma_samples)) >> (ctb_log2_size_y)) : ((sps_pic_height_max_in_luma_samples) + (1<<(ctb_log2_size_y)) - 1) >> (ctb_log2_size_y));
        const int wlen            = av_ceil_log2(tmp_width_val);
        const int hlen            = av_ceil_log2(tmp_height_val);
        unsigned int sps_subpic_id_len;
        int sps_subpic_same_size_flag = 0;
        int sps_independent_subpics_flag = 0;
        if (sps_num_subpics_minus1 > 0) {       // sps_num_subpics_minus1
            sps_independent_subpics_flag = bs.GetWord(1);
            sps_subpic_same_size_flag = bs.GetWord(1);
        }
        for (int i = 0; sps_num_subpics_minus1 > 0 && i <= sps_num_subpics_minus1; i++) {
            if (!sps_subpic_same_size_flag || i == 0) {
                if (i > 0 && sps_pic_width_max_in_luma_samples > ctb_size_y)
                    bs.GetWord(wlen);
                if (i > 0 && sps_pic_height_max_in_luma_samples > ctb_size_y)
                    bs.GetWord(hlen);
                if (i < sps_num_subpics_minus1 && sps_pic_width_max_in_luma_samples > ctb_size_y)
                    bs.GetWord(wlen);
                if (i < sps_num_subpics_minus1 && sps_pic_height_max_in_luma_samples > ctb_size_y)
                    bs.GetWord(hlen);
            }
            if (!sps_independent_subpics_flag) {
                bs.GetWord(2);       // sps_subpic_treated_as_pic_flag && sps_loop_filter_across_subpic_enabled_flag
            }
        }
        sps_subpic_id_len = bs.GetUE() + 1;
        if (bs.GetBit()) {    // sps_subpic_id_mapping_explicitly_signalled_flag
            if (bs.GetBit())  // sps_subpic_id_mapping_present_flag
                for (int i = 0; i <= sps_num_subpics_minus1; i++) {
                    bs.GetWord(sps_subpic_id_len); // sps_subpic_id[i]
                }
        }
    }
    int bit_depth_minus8 = bs.GetUE();

    /* nothing useful for vvcc past this point */
    return ;
}

H266Track::H266Track()
{

}

string H266Track::getSdp()
{
    stringstream ss;
    ss << "m=video 0 RTP/AVP " << payloadType_ << "\r\n"
    //    << "b=AS:" << bitrate << "\r\n"
       << "a=rtpmap:" << payloadType_ << " H266/" << samplerate_ << "\r\n"
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

void H266Track::getWidthAndHeight(int& width, int& height, int& fps)
{
    if (_width && _height) {
        width = _width;
        height = _height;
    }
	
	if (!_sps || !_pps) {
		logError << "no sps/pps";
		return ;
	}

    vvcParam params = {0};
    parseVvcSps((uint8_t*)(_sps->data() + _sps->startSize()), _sps->size() - _sps->startSize(), params);
    _width = params.width;
    _height = params.height;
    width = _width;
    height = _height;
}

string H266Track::getConfig()
{
    if (!_sps || !_pps) {
        return "";
    }

    string config;

    auto spsSize = _sps->startSize();
    auto vpsSize = _vps ? _vps->startSize() : 0;
    auto ppsSize = _pps->startSize();

    int vpsLen = _vps ? (_vps->size() - vpsSize) : 0;
    auto spsBuffer = _sps->_buffer;
    int spsLen = _sps->size() - spsSize;
    auto ppsBuffer = _pps->_buffer;
    int ppsLen = _pps->size() - ppsSize;

    config.resize(17  + vpsLen + spsLen + ppsLen);
    auto data = (char*)config.data();

    int num = 2;
    if (vpsLen > 0) {
        num = 3;
    }

    *data++ = 0b11111 << 3 | 0b11 << 1 | 0b0;
	// 1 byte
    // 默认vps，sps，pps 各一个
    *data++ = num; //numOfArrays
    //2 byte

    if (vpsLen > 0) {
        auto vpsBuffer = _vps->_buffer;
        *data++ = 1 << 7 | H266NalType::H266_VPS & 0x1f; //array_completeness(1),reserved(2),NAL_unit_type(5)
        // 3 byte

        // 一个vps
        *data++ = 0; //numNalus
        *data++ = 1;
        // 5 byte

        *data++ = (uint8_t)(vpsLen >> 8); //sequence parameter set length high 8 bits
        *data++ = (uint8_t)(vpsLen); //sequence parameter set  length low 8 bits
        // 7 byte

        memcpy(data, vpsBuffer->data() + vpsSize, vpsLen); //H264 sequence parameter set
        data += vpsLen; 
        // 7 + vpsLen
    }

    *data++ = 1 << 7 | H266NalType::H266_SPS & 0x1f;
	// 8 byte + vpsLen

    // 一个sps
    *data++ = 0;
    *data++ = 1;
	// 10 byte + vpsLen

    *data++ = (uint8_t)(spsLen >> 8); //sequence parameter set length high 8 bits
    *data++ = (uint8_t)(spsLen); //sequence parameter set  length low 8 bits
	// 12 byte + vpsLen

    memcpy(data, spsBuffer->data() + spsSize, spsLen); //H264 sequence parameter set
    data += spsLen; 
    // 12 + vpsLen + spsLen

    *data++ = 1 << 7 | H266NalType::H266_PPS & 0x1f;
	// 13 + vpsLen + spsLen

    // 一个pps
    *data++ = 0;
    *data++ = 1;
	// 15 + vpsLen + spsLen

    *data++ = (uint8_t)(ppsLen >> 8); //sequence parameter set length high 8 bits
    *data++ = (uint8_t)(ppsLen); //sequence parameter set  length low 8 bits
	// 17 + vpsLen + spsLen

    memcpy(data, ppsBuffer->data() + ppsSize, ppsLen); //H264 sequence parameter set
    data += ppsLen; 
    // 17  + vpsLen + spsLen + ppsLen

    return config;
}

void H266Track::setConfig(const string& config)
{
    if (config.size() < 17) {
        return ;
    } //17

    _hvcc = config;

    NALBitstream bs((void*)_hvcc.data(), _hvcc.size());
    bs.GetWord(5); // reserved '11111'b
    bs.GetWord(2); // lengthSizeMinusOne
    uint8_t ptl_present_flag = bs.GetBit();
    if (ptl_present_flag) {
        bs.GetWord(9); //ols_idx
        int num_sublayers = bs.GetWord(3); //num_sublayers
        bs.GetWord(2); //constant_frame_rate
        bs.GetWord(2); //chroma_format_idc
        bs.GetWord(3); //bit_depth_minus8
        bs.GetWord(5); //reserved '11111'b
        {
            // mpeg4_vvc_ptl_record_load
            bs.GetWord(2); // reserved '11'b
            int num_bytes_constraint_info = bs.GetWord(6);
            bs.GetWord(7); // general_profile_idc
            bs.GetWord(1); // general_tier_flag
            bs.GetWord(8); // general_level_idc
            for (int i = 0; i < num_bytes_constraint_info; ++i) {
                bs.GetWord(8); // general_constraint_info[i]
            }
            int ptl_sublayer_level_present_flag = 0;
            // for (int i = num_sublayers - 2; i >= 0; --i) {
                ptl_sublayer_level_present_flag = bs.GetWord(8);
            // }
            for (int i = num_sublayers - 2; i >= 0; --i) {
                if (ptl_sublayer_level_present_flag & (1 << i)) {
                    bs.GetWord(8); // sublayer_level_idc
                }
            }
            int ptl_num_sub_profiles = bs.GetWord(8);
            for (int i = 0; i < ptl_num_sub_profiles; ++i) {
                bs.GetWord(32); // general_sub_profile_idc[i]
            }
        }
        _width = bs.GetWord(16); //max_picture_width
        _height = bs.GetWord(16); //max_picture_height
        bs.GetWord(16); //avg_frame_rate
    }

    int numOfArr = bs.GetWord(8);
    for (int i = 0; i < numOfArr; ++i) {
        uint8_t nalType = bs.GetWord(8);
        int num = 1;
        if ((nalType & 0x1F) != H266_DCI && (nalType & 0x1F) != H266_OPI) {
            num = bs.GetWord(16);
        }

        for (int j = 0; j < num; ++j) {
            int naluLen = bs.GetWord(16);
            auto frame = make_shared<H266Frame>();
			frame->_startSize = 4;
			frame->_codec = "h266";
			frame->_index = index_;
			frame->_trackType = VideoTrackType;

			frame->_buffer->assign("\x00\x00\x00\x01", 4);
			frame->_pts = frame->_dts = 0;
            int bitIndex = (bs.getBitPos() + 1) / 8 - 1;
			frame->_buffer->append((char*)_hvcc.data() + bitIndex, naluLen);
			bs.skipByte(naluLen);

			logInfo << "get a config frame: " << (int)frame->getNalType();
			if (frame->getNalType() == H266_VPS) {
				setVps(frame);
			} else if (frame->getNalType() == H266_SPS) {
				setSps(frame);
			} else if (frame->getNalType() == H266_PPS) {
				setPps(frame);
			}
        }
    }
}

bool H266Track::isBFrame(unsigned char* data, int size)
{
	if (!data || !_sps || !_pps) {
		return false;
	}
	// de_emulation_prevention(data, (unsigned int *)&size);

	

	return false;
}

H266Track::Ptr H266Track::createTrack(int index, int payloadType, int samplerate)
{
    auto trackInfo = make_shared<H266Track>();
    trackInfo->index_ = index;
    trackInfo->codec_ = "h266";
    trackInfo->payloadType_ = payloadType;
    trackInfo->trackType_ = "video";
    trackInfo->samplerate_ = samplerate;

    return trackInfo;
}

void H266Track::registerTrackInfo()
{
	TrackInfo::registerTrackInfo("h266", [](int index, int payloadType, int samplerate){
		auto trackInfo = make_shared<H266Track>();
		trackInfo->index_ = VideoTrackType;
		trackInfo->codec_ = "h266";
		trackInfo->payloadType_ = 96;
		trackInfo->trackType_ = "video";
		trackInfo->samplerate_ = 90000;

		return trackInfo;
	});
}

void H266Track::onFrame(const FrameBuffer::Ptr& frame)
{
	if (_sps && _pps) {
		_hasReady = true;
		return ;
	}

	if (!frame || frame->size() < 5) {
		return ;
	}

	if (frame->getNalType() == H266_VPS) {
        setVps(frame);
    } else if (frame->getNalType() == H266_SPS) {
        setSps(frame);
    } else if (frame->getNalType() == H266_PPS) {
        setPps(frame);
    }
}