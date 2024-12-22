#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <cmath>
#include<string.h>

#include "AV1Track.h"
#include "AV1Frame.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/Base64.h"

using namespace std;

class ObuBitstream
{
public:
	ObuBitstream() : m_data(NULL), m_len(0), m_idx(0), m_bits(0), m_byte(0), m_zeros(0) {};
	ObuBitstream(void * data, int len) { Init(data, len); };
	void Init(void * data, int len)
	{
		m_data = (unsigned char*)data;
		m_len = len;
		m_idx = 0;
		m_bits = 0;
		m_byte = 0;
		m_zeros = 0;
	};
 
	uint8_t GetBYTE()
	{
		if (m_idx >= m_len)
			return 0;
		uint8_t b = m_data[m_idx++];
		// to avoid start-code emulation, a byte 0x03 is inserted
		// after any 00 00 pair. Discard that here.
		if (b == 0)
		{
			m_zeros++;
			if ((m_idx < m_len) && (m_zeros == 2) && (m_data[m_idx] == 0x03))
			{
 
				m_idx++;
				m_zeros = 0;
			}
		}
		else {
			m_zeros = 0;
		}
		return b;
	};
 
	uint32_t GetBit()
	{
		if (m_bits == 0)
		{
			m_byte = GetBYTE();
			m_bits = 8;
		}
		m_bits--;
		return (m_byte >> m_bits) & 0x1;
	};
 
	uint32_t GetWord(int bits)
	{
 
		uint32_t u = 0;
		while (bits > 0)
		{
			u <<= 1;
			u |= GetBit();
			bits--;
		}
		return u;
	};

    uint64_t GetUvlc() {
        uint64_t value;
        int leadingZeros;
        for (leadingZeros = 0; !GetBit(); ++leadingZeros)
        {
        }

        if (leadingZeros >= 32)
            return (1ULL << 32) - 1;
        
        value = GetWord(leadingZeros);
        return (1ULL << leadingZeros) - 1 + value;
    }
 
 
private:
	unsigned char* m_data;
	int m_len;
	int m_idx;
	int m_bits;
	uint8_t m_byte;
	int m_zeros;
};

void parserSequenceHeader(const uint8_t* data, size_t bytes, AV1Track* track) {
    uint8_t decoder_model_info_present_flag = 0;

    ObuBitstream bs((unsigned char*)data, bytes);
    track->info.seq_profile = bs.GetWord(3);
    bs.GetWord(1); // still_picture
    uint8_t reduced_still_picture_header = bs.GetWord(1);
    if (reduced_still_picture_header) { 
        track->info.initial_presentation_delay_present = 0;
        track->info.seq_level_idx_0 = bs.GetWord(5);
        track->info.seq_tier_0 = 0;
        decoder_model_info_present_flag = 0;
    } else {
        if (bs.GetWord(1)) //timing_info_present_flag
        {
            // timing_info( )
            bs.GetWord(32); // num_units_in_display_tick
            bs.GetWord(32); // time_scale
            if(bs.GetWord(1)) // equal_picture_interval
                bs.GetUvlc(); // num_ticks_per_picture_minus_1

            decoder_model_info_present_flag = bs.GetBit();
            if (decoder_model_info_present_flag) {
                // decoder_model_info( )
                track->info.buffer_delay_length_minus_1 = bs.GetWord(5); // buffer_delay_length_minus_1
                bs.GetWord(32); // num_units_in_decoding_tick
                bs.GetWord(5); // buffer_removal_time_length_minus_1
                bs.GetWord(5); // frame_presentation_time_length_minus_1
            }

        } else {
            decoder_model_info_present_flag = 0;
        }
    }

    track->info.initial_presentation_delay_present = bs.GetBit();
    uint8_t operating_points_cnt_minus_1  = bs.GetWord(5);
    for (int i = 0; i <= operating_points_cnt_minus_1; i++)
    {
        uint8_t seq_level_idx;
        uint8_t seq_tier;
        uint8_t initial_display_delay_minus_1;

        bs.GetWord(12); // operating_point_idc[ i ]
        seq_level_idx = bs.GetWord(5); // seq_level_idx[ i ]
        if (seq_level_idx > 7)
        {
            seq_tier = bs.GetWord(1); // seq_tier[ i ]
        }
        else
        {
            seq_tier = 0;
        }

        if (decoder_model_info_present_flag)
        {
            if (bs.GetBit()) // decoder_model_present_for_this_op[i]
            {
                uint8_t n;
                n = track->info.buffer_delay_length_minus_1 + 1;
                bs.GetWord(n); // decoder_buffer_delay[ i ]
                bs.GetWord(n); // encoder_buffer_delay[ i ]
                bs.GetBit(); // low_delay_mode_flag[ i ]
            }
        }

        if (track->info.initial_presentation_delay_present && bs.GetBit()) // initial_display_delay_present_for_this_op[ i ]
            initial_display_delay_minus_1 = bs.GetWord(4); // initial_display_delay_minus_1[ i ]
        else
            initial_display_delay_minus_1 = 0;

        if (0 == i)
        {
            track->info.seq_level_idx_0 = seq_level_idx;
            track->info.seq_tier_0 = seq_tier;
            track->info.initial_presentation_delay_minus_one = initial_display_delay_minus_1;
        }
    }

	// choose_operating_point( )
	uint8_t frame_width_bits_minus_1 = bs.GetWord(4);
	uint8_t frame_height_bits_minus_1 = bs.GetWord(4);
	track->info.width = 1 + bs.GetWord(frame_width_bits_minus_1 + 1); // max_frame_width_minus_1
	track->info.height = 1 + bs.GetWord(frame_height_bits_minus_1 + 1); // max_frame_height_minus_1

	if (!reduced_still_picture_header && bs.GetBit()) // frame_id_numbers_present_flag
	{
		bs.GetWord(4); // delta_frame_id_length_minus_2
		bs.GetWord(3); // additional_frame_id_length_minus_1
	}

	bs.GetBit(); // use_128x128_superblock
	bs.GetBit(); // enable_filter_intra
	bs.GetBit(); // enable_intra_edge_filter

    uint8_t enable_order_hint = 0;
    uint8_t seq_force_screen_content_tools = 0;
	if (!reduced_still_picture_header)
	{
		bs.GetBit(); // enable_interintra_compound
		bs.GetBit(); // enable_masked_compound
		bs.GetBit(); // enable_warped_motion
		bs.GetBit(); // enable_dual_filter
		enable_order_hint = bs.GetWord(1);
		if (enable_order_hint)
		{
			bs.GetBit(); // enable_jnt_comp
			bs.GetBit(); // enable_ref_frame_mvs
		}
		if (bs.GetBit()) // seq_choose_screen_content_tools
		{
			seq_force_screen_content_tools = 2; // SELECT_SCREEN_CONTENT_TOOLS;
		}
		else
		{
			seq_force_screen_content_tools = bs.GetWord(1); // seq_force_screen_content_tools
		}

		if (seq_force_screen_content_tools > 0)
		{
			if (!bs.GetBit()) // seq_choose_integer_mv
				bs.GetBit(); // seq_force_integer_mv
			//else
			// seq_force_integer_mv = SELECT_INTEGER_MV
		}
		else
		{
			//seq_force_integer_mv = SELECT_INTEGER_MV;
		}

		if (enable_order_hint)
		{
			bs.GetWord(3); // order_hint_bits_minus_1
		}
	}

	bs.GetBit(); // enable_superres
	bs.GetBit(); // enable_cdef
	bs.GetBit(); // enable_restoration

	// color_config( )
    // 暂时不解析了
	// aom_av1_color_config(&bits, av1);

	// bs.GetBit(); // film_grain_params_present
}

AV1Track::AV1Track()
{

}

string AV1Track::getSdp()
{
    stringstream ss;
    ss << "m=video 0 RTP/AVP " << payloadType_ << "\r\n"
    //    << "b=AS:" << bitrate << "\r\n"
       << "a=rtpmap:" << payloadType_ << " AV1/" << samplerate_ << "\r\n"
       << "a=control:trackID=" << index_ << "\r\n";

	return ss.str();
}

void AV1Track::getWidthAndHeight(int& width, int& height, int& fps)
{
    if (info.height && info.width) {
        width = info.width;
        height = info.height;

        return ;
    }

    if (!_sequence) {
        return ;
    }

    parserSequenceHeader((uint8_t *)_sequence->data(), _sequence->size(), this);

    width = info.width;
    height = info.height;
}

string AV1Track::getConfig()
{
    if (!_sequence) {
        return "";
    }

    string config;

    // data[0] = (uint8_t)((av1->marker << 7) | av1->version);
	// data[1] = (uint8_t)((av1->seq_profile << 5) | av1->seq_level_idx_0);
	// data[2] = (uint8_t)((av1->seq_tier_0 << 7) | (av1->high_bitdepth << 6) | (av1->twelve_bit << 5) | (av1->monochrome << 4) | (av1->chroma_subsampling_x << 3) | (av1->chroma_subsampling_y << 2) | av1->chroma_sample_position);
	// data[3] = (uint8_t)((av1->initial_presentation_delay_present << 4) | av1->initial_presentation_delay_minus_one);

    // 按照标准太麻烦了，直接乱写
    config.resize(_sequence->size() + 4);
    auto data = (char*)config.data();
    data[0] = 1 << 7 | 1;
    data[1] = 0xff;
    data[2] = 0xff;
    data[3] = 0xff;

    memcpy(data + 4, _sequence->data(), _sequence->size());

    return config;
}

bool AV1Track::isBFrame(unsigned char* data, int size)
{
	return false;
}

AV1Track::Ptr AV1Track::createTrack(int index, int payloadType, int samplerate)
{
    auto trackInfo = make_shared<AV1Track>();
    trackInfo->index_ = index;
    trackInfo->codec_ = "av1";
    trackInfo->payloadType_ = payloadType;
    trackInfo->trackType_ = "video";
    trackInfo->samplerate_ = samplerate;

    return trackInfo;
}

void AV1Track::registerTrackInfo()
{
	TrackInfo::registerTrackInfo("av1", [](int index, int payloadType, int samplerate){
		auto trackInfo = make_shared<AV1Track>();
		trackInfo->index_ = VideoTrackType;
		trackInfo->codec_ = "av1";
		trackInfo->payloadType_ = 96;
		trackInfo->trackType_ = "video";
		trackInfo->samplerate_ = 90000;

		return trackInfo;
	});
}

void AV1Track::onFrame(const FrameBuffer::Ptr& frame)
{
	if (_sequence) {
		_hasReady = true;
		return ;
	}

	if (!frame || frame->size() < 5) {
		return ;
	}

    auto av1Frame = dynamic_pointer_cast<AV1Frame>(frame);
	if (av1Frame->getObuType() == AV1OBUType::OBU_SEQUENCE_HEADER) {
        setVps(frame);
    }
}