#include "H264Deserialize.h"

#include <cassert>
#include <cstdint>
#include <memory>

#include "H264Common.h"
#include "H26xUltis.h"

namespace Mmp
{
namespace Codec
{

// Quick Note
//
// 1) IdrPicFlag : ( ( nal_unit_type = = 5 ) ? 1 : 0 ) (7-0)
//
// 2) pic_parameter_set_id
//   pic_parameter_set_id identifies the picture parameter set that is referred to in the slice header. The value of
// pic_parameter_set_id shall be in the range of 0 to 255, inclusive.
// 
//  3) seq_parameter_set_id
//   seq_parameter_set_id refers to the active sequence parameter set. The value of seq_parameter_set_id shall be in the 
// range of 0 to 31, inclusive.
//
//  4) ChromaArrayType
//    Depending on the value of separate_colour_plane_flag, the value of the variable ChromaArrayType is assigned as 
//    follows:
//    - If separate_colour_plane_flag is equal to 0, ChromaArrayType is set equal to chroma_format_idc.
//    - Otherwise (separate_colour_plane_flag is equal to 1), ChromaArrayType is set equal to 0.
//

// Table 7-3 – Specification of default scaling lists Default_4x4_Intra and Default_4x4_Inter
static std::vector<int32_t> Default_4x4_Intra = {6, 13, 13, 20, 20, 20, 28, 28, 28, 28, 32, 32, 32, 37, 37, 42};
static std::vector<int32_t> Default_4x4_Inter = {10, 14, 14, 20, 20, 20, 24, 24, 24, 24, 27, 27, 27, 30, 30, 34};
// Table 7-4 – Specification of default scaling lists Default_8x8_Intra and Default_8x8_Inter
static std::vector<int32_t> Default_8x8_Intra = {6, 10, 10, 13, 11, 13, 16, 16, 16, 16, 18, 18, 18, 18, 18, 23,
                                                 23, 23, 23, 23, 23, 25, 25, 25, 25, 25, 25, 25, 27, 27, 27, 27,
                                                 27, 27, 27, 27, 29, 29, 29, 29, 29, 29, 29, 31, 31, 31, 31, 31,
                                                 31, 33, 33, 33, 33, 33, 36, 36, 36, 36, 38, 38, 38, 40, 40, 42
                                                };
static std::vector<int32_t> Default_8x8_Inter  = {10, 14, 14, 20, 20, 20, 24, 24, 24, 24, 27, 27, 27, 30, 30, 34,
                                                 9, 13, 13, 15, 13, 15, 17, 17, 17, 17, 19, 19, 19, 19, 19, 21,
                                                 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 27, 27, 27, 27, 27,
                                                 27, 28, 28, 28, 28, 28, 30, 30, 30, 30, 32, 32, 32, 33, 33, 35
                                                };

H264Deserialize::H264Deserialize()
{
    _contex = std::make_shared<H264ContextSyntax>();
}

H264Deserialize::~H264Deserialize()
{

}

bool H264Deserialize::DeserializeByteStreamNalUnit(H26xBinaryReader::ptr br, H264NalSyntax::ptr nal)
{
    // See also : ISO 14496/10(2020) - B.1.1 Byte stream NAL unit syntax
    try
    {
        uint32_t next_24_bits = 0;
        br->U(24, next_24_bits, true);
        while (next_24_bits != 0x000001)
        {
            if ((next_24_bits & 0xFFFF) == 0)
            {
                br->Skip(8);
            }
            else if ((next_24_bits & 0xFF) == 0)
            {
                br->Skip(16);
            }
            else
            {
                br->Skip(32);
            }
            br->U(24, next_24_bits, true);
        }
        br->Skip(24); // start_code_prefix_one_3bytes /* equal to 0x000001 */
        if (!DeserializeNalSyntax(br, nal))
        {
            return false;
        }
        while (br->more_data_in_byte_stream())
        {
            br->U(24, next_24_bits, true);
            if (next_24_bits != 0x000001)
            {
                if ((next_24_bits & 0xFFFF) == 0)
                {
                    br->Skip(8);
                }
                else if ((next_24_bits & 0xFF) == 0)
                {
                    br->Skip(16);
                }
                else
                {
                    br->Skip(32);
                }
            }
            else
            {
                break;
            }
        }
        return true;
    }
    catch (const std::out_of_range& /* eof */)
    {
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeNalSyntax(H26xBinaryReader::ptr br, H264NalSyntax::ptr nal)
{
    // See also : ISO 14496/10(2020) - 7.3.1 NAL unit syntax
    try
    {
        uint8_t  forbidden_zero_bit = 0;
        br->BeginNalUnit();
        br->U(1, forbidden_zero_bit);
        MPP_H26X_SYNTAXT_STRICT_CHECK(forbidden_zero_bit == 0, "[nal] forbidden_zero_bit should be 0", return false);
        br->U(2, nal->nal_ref_idc);
        br->U(5, nal->nal_unit_type);
        if (nal->nal_unit_type == H264NaluType::MMP_H264_NALU_TYPE_PREFIX || nal->nal_unit_type == H264NaluType::MMP_H264_NALU_TYPE_SLC_EXT ||
            nal->nal_unit_type == H264NaluType::MMP_H264_NALU_TYPE_VDRD
        )
        {
            if (nal->nal_unit_type != H264NaluType::MMP_H264_NALU_TYPE_VDRD)
            {
                br->U(1, nal->svc_extension_flag);
            }
            else
            {
                br->U(1, nal->avc_3d_extension_flag);
            }
            if (nal->svc_extension_flag)
            {
                nal->svc = std::make_shared<H264NalSvcSyntax>();
                if (!DeserializeNalSvcSyntax(br, nal->svc))
                {
                    return false;
                }
            }
            else if (nal->avc_3d_extension_flag)
            {
                nal->avc = std::make_shared<H264Nal3dAvcSyntax>();
                if (!DeserializeNal3dAvcSyntax(br, nal->avc))
                {
                    return false;
                }
            }
            else
            {
                nal->mvc = std::make_shared<H264NalMvcSyntax>();
                if (!DeserializeNalMvcSyntax(br, nal->mvc))
                {
                    return false;
                }
            }
        }
        switch (nal->nal_unit_type)
        {        
            case H264NaluType::MMP_H264_NALU_TYPE_SPS:
            {
                nal->sps = std::make_shared<H264SpsSyntax>();
                if (!DeserializeSpsSyntax(br, nal->sps))
                {
                    assert(false);
                    return false;
                }
                break;
            }
            case H264NaluType::MMP_H264_NALU_TYPE_PPS:
            {
                nal->pps = std::make_shared<H264PpsSyntax>();
                if (!DeserializePpsSyntax(br, nal->pps))
                {
                    assert(false);
                    return false;
                }
                break;
            }
            case H264NaluType::MMP_H264_NALU_TYPE_IDR: /* pass through */
            case H264NaluType::MMP_H264_NALU_TYPE_SLICE:
            {
                // Hint : Slice = Slice header + Slice data + rbsp_trailing_bits()
                //        only parse slice header and may move to next nal unit
                nal->slice = std::make_shared<H264SliceHeaderSyntax>();
                if (!DeserializeSliceHeaderSyntax(br, nal, nal->slice))
                {
                    assert(false);
                    return false;
                }
                br->MoveNextByte();
                break;
            }
            case H264NaluType::MMP_H264_NALU_TYPE_SEI:
            {
                nal->sei = std::make_shared<H264SeiSyntax>();
                if (!DeserializeSeiSyntax(br, nal->sei))
                {
                    assert(false);
                    return false;
                }
                break;
            }
            default:
                std::string msg = "unspport nal type parse " + std::to_string(nal->nal_unit_type);
                MPP_H26X_SYNTAXT_NORMAL_CHECK(true, msg, ;);
                break;
        }
        br->EndNalUnit();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeHrdSyntax(H26xBinaryReader::ptr br, H264HrdSyntax::ptr hrd)
{
    // See also : ISO 14496/10(2020) - E.1.2 HRD parameters syntax
    try
    {
        br->UE(hrd->cpb_cnt_minus1);
        {
            // Hint : cpb_cnt_minus1 plus 1 specifies the number of alternative CPB specifications in the bitstream. The value of 
            // cpb_cnt_minus1 shall be in the range of 0 to 31, inclusive. When low_delay_hrd_flag is equal to 1, cpb_cnt_minus1 shall 
            // be equal to 0. When cpb_cnt_minus1 is not present, it shall be inferred to be equal to 0.
            MPP_H26X_SYNTAXT_STRICT_CHECK(hrd->cpb_cnt_minus1 >= 0 && hrd->cpb_cnt_minus1 < 31, "[hrd] cpb_cnt_minus1 out of range", return false);
        }
        br->U(4, hrd->bit_rate_scale);
        br->U(4, hrd->cpb_size_scale);
        {
            hrd->bit_rate_value_minus1.resize(hrd->cpb_cnt_minus1 + 1);
            hrd->cpb_size_value_minus1.resize(hrd->cpb_cnt_minus1 + 1);
            hrd->cbr_flag.resize(hrd->cpb_cnt_minus1 + 1);
        }
        for (uint32_t SchedSelIdx=0; SchedSelIdx<=hrd->cpb_cnt_minus1; SchedSelIdx++)
        {
            br->UE(hrd->bit_rate_value_minus1[SchedSelIdx]);
            br->UE(hrd->cpb_size_value_minus1[SchedSelIdx]);
            br->U(1, hrd->cbr_flag[SchedSelIdx]);
        }
        br->U(5, hrd->initial_cpb_removal_delay_length_minus1);
        br->U(5, hrd->cpb_removal_delay_length_minus1);
        br->U(5, hrd->dpb_output_delay_length_minus1);
        br->U(5, hrd->time_offset_length);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeVuiSyntax(H26xBinaryReader::ptr br, H264VuiSyntax::ptr vui)
{
    constexpr uint8_t Extended_SAR = 255; // Table E-1 – Meaning of sample aspect ratio indicator

    static auto getSar = [](uint32_t aspect_ratio_idc, uint16_t& sar_width, uint16_t& sar_height) -> void
    {
        // See also : ISO 14496/10(2020) - Table E-1 – Meaning of sample aspect ratio indicator
        #define MMP_ASPECT_RATION_IDC_MAP(_aspect_ratio_idc, _sar_width, _sar_height) case _aspect_ratio_idc:\
                                                                                      {\
                                                                                          sar_width = _sar_width;\
                                                                                          sar_height = _sar_height;\
                                                                                          break;\
                                                                                      }
        switch (aspect_ratio_idc)
        {
            MMP_ASPECT_RATION_IDC_MAP(1, 1, 1);
            MMP_ASPECT_RATION_IDC_MAP(2, 12, 11);
            MMP_ASPECT_RATION_IDC_MAP(3, 10, 11);
            MMP_ASPECT_RATION_IDC_MAP(4, 16, 11);
            MMP_ASPECT_RATION_IDC_MAP(5, 40, 33);
            MMP_ASPECT_RATION_IDC_MAP(6, 24, 11);
            MMP_ASPECT_RATION_IDC_MAP(7, 20, 11);
            MMP_ASPECT_RATION_IDC_MAP(8, 32, 11);
            MMP_ASPECT_RATION_IDC_MAP(9, 80, 33);
            MMP_ASPECT_RATION_IDC_MAP(10, 18, 11);
            MMP_ASPECT_RATION_IDC_MAP(11, 15, 11);
            MMP_ASPECT_RATION_IDC_MAP(12, 64, 33);
            MMP_ASPECT_RATION_IDC_MAP(13, 160, 99);
            MMP_ASPECT_RATION_IDC_MAP(14, 4, 3);
            MMP_ASPECT_RATION_IDC_MAP(15, 3, 2);
            MMP_ASPECT_RATION_IDC_MAP(16, 2, 1);
            default:
                assert(false);
                sar_width = 1;
                sar_height = 1;
                break;
        }
        #undef MMP_ASPECT_RATION_IDC_MAP
    };

    // See also : ISO 14496/10(2020) - E.1.1 VUI parameters syntax
    try
    {
        br->U(1, vui->aspect_ratio_info_present_flag);
        if (vui->aspect_ratio_info_present_flag)
        {
            br->U(8, vui->aspect_ratio_idc);
            if (vui->aspect_ratio_idc >=1 && vui->aspect_ratio_idc<=16)
            {
                getSar(vui->aspect_ratio_idc, vui->sar_width, vui->sar_height);
            }
            else if (vui->aspect_ratio_idc == Extended_SAR)
            {
                br->U(16, vui->sar_width);
                br->U(16, vui->sar_height);
            }
        }
        else
        {
            // Reference : FFmpeg 6.x
            vui->sar_width = 0;
            vui->sar_height = 1;
        }
        br->U(1, vui->overscan_info_present_flag);
        if (vui->overscan_info_present_flag)
        {
            br->U(1, vui->overscan_appropriate_flag);
        }
        br->U(1, vui->video_signal_type_present_flag);
        if (vui->video_signal_type_present_flag)
        {
            br->U(3, vui->video_format);
            br->U(1, vui->video_full_range_flag);
            br->U(1, vui->colour_description_present_flag);
            if (vui->colour_description_present_flag)
            {
                br->U(8, vui->colour_primaries);
                br->U(8, vui->transfer_characteristics);
                br->U(8, vui->matrix_coefficients);
            }
            else
            {
                // See also : Table E-5 – Matrix coefficients interpretation using matrix_coefficients syntax elemen
                vui->transfer_characteristics = 2; /* Unspecified */
            }
        }
        br->U(1, vui->chroma_location_info_present_flag);
        if (vui->chroma_location_info_present_flag)
        {
            br->UE(vui->chroma_sample_loc_type_top_field);
            br->UE(vui->chroma_sample_loc_type_bottom_field);
        }
        br->U(1, vui->timing_info_present_flag);
        if (vui->timing_info_present_flag)
        {
                br->U(32, vui->num_units_in_tick);
                br->U(32, vui->time_scale);
                // Reference : FFmpeg 6.x
                if (!vui->num_units_in_tick || !vui->time_scale)
                {
                    vui->timing_info_present_flag = 0;
                }
                br->U(1, vui->fixed_frame_rate_flag);   
        }
        br->U(1, vui->nal_hrd_parameters_present_flag);
        if (vui->nal_hrd_parameters_present_flag)
        {
            vui->nal_hrd_parameters = std::make_shared<H264HrdSyntax>();
            if (!DeserializeHrdSyntax(br, vui->nal_hrd_parameters))
            {
                return false;
            }
        }
        br->U(1, vui->vcl_hrd_parameters_present_flag);
        if (vui->vcl_hrd_parameters_present_flag)
        {
            vui->vcl_hrd_parameters = std::make_shared<H264HrdSyntax>();
            if (!DeserializeHrdSyntax(br, vui->vcl_hrd_parameters))
            {
                return false;
            }
        }
        if (vui->nal_hrd_parameters_present_flag || vui->vcl_hrd_parameters_present_flag)
        {
            br->U(1, vui->low_delay_hrd_flag);
        }
        br->U(1, vui->pic_struct_present_flag);
        br->U(1, vui->bitstream_restriction_flag);
        if (vui->bitstream_restriction_flag)
        {
            br->U(1, vui->motion_vectors_over_pic_boundaries_flag);
            br->UE(vui->max_bytes_per_pic_denom);
            br->UE(vui->max_bits_per_mb_denom);
            br->UE(vui->log2_max_mv_length_horizontal);
            br->UE(vui->log2_max_mv_length_vertical);
            br->UE(vui->num_reorder_frames);
            br->UE(vui->max_dec_frame_buffering);
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeSeiSyntax(H26xBinaryReader::ptr br, H264SeiSyntax::ptr sei)
{
    // See also : ISO 14496/10(2020) - 7.3.2.3.1 Supplemental enhancement information message syntax
    try
    {
        uint8_t ff_byte = 0;
        do
        {
            br->U(8, ff_byte);
            sei->payloadType += ff_byte;
        } while (ff_byte == 0xFF);

        do
        {
            br->U(8, ff_byte);
            sei->payloadSize += ff_byte;
        } while (ff_byte == 0xFF);

        switch (sei->payloadType) 
        {
            // See also : ISO 14496/10(2020) - D.1.1 General SEI message syntax
            case H264SeiType::MMP_H264_SEI_BUFFERING_PERIOD:
            {
                sei->bp = std::make_shared<H264SeiBufferPeriodSyntax>();
                if (!DeserializeSeiBufferPeriodSyntax(br, sei->bp))
                {
                    return false;
                }
                break;
            }
            case H264SeiType::MMP_H264_SEI_PIC_TIMING:
            {
                H264VuiSyntax::ptr vui = _contex->sps && _contex->sps->vui_parameters_present_flag ? _contex->sps->vui_seq_parameters : nullptr;
                sei->pt = std::make_shared<H264SeiPictureTimingSyntax>();
                MPP_H26X_SYNTAXT_STRICT_CHECK(vui, "[sei] missing vui", return false);
                if (!DeserializeSeiPictureTimingSyntax(br, vui, sei->pt))
                {
                    return false;
                }
                break;
            }
            case H264SeiType::MMP_H264_SEI_USER_DATA_REGISTERED_ITU_T_T35:
            {
                sei->udr = std::make_shared<H264SeiUserDataRegisteredSyntax>();
                if (!DeserializeSeiUserDataRegisteredSyntax(br, (uint32_t)sei->payloadSize, sei->udr))
                {
                    return false;
                }
                break;
            }
            case H264SeiType::MMP_H264_SEI_USER_DATA_UNREGISTERED:
            {
                sei->udn = std::make_shared<H264SeiUserDataUnregisteredSyntax>();
                if (!DeserializeSeiUserDataUnregisteredSyntax(br, (uint32_t)sei->payloadSize, sei->udn))
                {
                    return false;
                }
                break;
            }
            case H264SeiType::MMP_H264_SEI_RECOVERY_POINT:
            {
                sei->rp = std::make_shared<H264SeiRecoveryPointSyntax>();
                if (!DeserializeSeiRecoveryPointSyntax(br, sei->rp))
                {
                    return false;
                }
                break;
            }
            case H264SeiType::MMP_H264_SEI_CONTENT_LIGHT_LEVEL_INFO:
            {
                sei->clli = std::make_shared<H264SeiContentLigntLevelInfoSyntax>();
                if (!DeserializeSeiContentLigntLevelInfoSyntax(br, sei->clli))
                {
                    return false;
                }
                break;
            }
            case H264SeiType::MMP_H264_SEI_DISPLAY_ORIENTATION:
            {
                sei->dot = std::make_shared<H264SeiDisplayOrientationSyntax>();
                if (!DeserializeSeiDisplayOrientationSyntax(br, sei->dot))
                {
                    return false;
                }
                break;
            }
            case H264SeiType::MMP_H264_SEI_FILM_GRAIN_CHARACTERISTICS:
            {
                sei->fg = std::make_shared<H264SeiFilmGrainSyntax>();
                if (!DeserializeSeiFilmGrainSyntax(br, sei->fg))
                {
                    return false;
                }
                break;
            }
            case H264SeiType::MMP_H264_SEI_FRAME_PACKING_ARRANGEMENT:
            {
                sei->fpa = std::make_shared<H264SeiFramePackingArrangementSyntax>();
                if (!DeserializeSeiFramePackingArrangementSyntax(br, sei->fpa))
                {
                    return false;
                }
                break;
            }
            case H264SeiType::MMP_H264_SEI_ALTERNATIVE_TRANSFER_CHARACTERISTICS:
            {
                sei->atc = std::make_shared<H264SeiAlternativeTransferCharacteristicsSyntax>();
                if (!DeserializeSeiAlternativeTransferCharacteristicsSyntax(br, sei->atc))
                {
                    return false;
                }
                break;
            }
            case H264SeiType::MP_H264_SEI_AMBIENT_VIEWING_ENVIRONMENT:
            {
                sei->awe = std::make_shared<H264AmbientViewingEnvironmentSyntax>();
                if (!DeserializeAmbientViewingEnvironmentSyntax(br, sei->awe))
                {
                    return false;
                }
                break;
            }
            case H264SeiType::MMP_H264_SEI_MASTERING_DISPLAY_COLOUR_VOLUME:
            {
                sei->mpvc = std::make_shared<H264MasteringDisplayColourVolumeSyntax>();
                if (!DeserializeSeiMasteringDisplayColourVolumeSyntax(br, sei->mpvc))
                {
                    return false;
                }
                break;
            }
            default:
                br->Skip(sei->payloadSize * 8);
                break;
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeSpsSyntax(H26xBinaryReader::ptr br, H264SpsSyntax::ptr sps)
{
    // See also : ISO 14496/10(2020) - 7.3.2.1.1 Sequence parameter set data syntax
    try
    {
        uint8_t reserved_zero_2bits = 0;
        br->U(8, sps->profile_idc);
        br->U(1, sps->constraint_set0_flag);
        br->U(1, sps->constraint_set1_flag);
        br->U(1, sps->constraint_set2_flag);
        br->U(1, sps->constraint_set3_flag);
        br->U(1, sps->constraint_set4_flag);
        br->U(1, sps->constraint_set5_flag);
        br->U(2, reserved_zero_2bits);
        br->U(8, sps->level_idc);
        br->UE(sps->seq_parameter_set_id);
        if (sps->profile_idc == 100 || sps->profile_idc == 110 ||
            sps->profile_idc == 122 || sps->profile_idc == 244 || sps->profile_idc == 44 ||
            sps->profile_idc == 83 || sps->profile_idc == 86 || sps->profile_idc == 118 ||
            sps->profile_idc == 128 || sps->profile_idc == 138 || sps->profile_idc == 139 ||
            sps->profile_idc == 134 || sps->profile_idc == 135
        )
        {
            br->UE(sps->chroma_format_idc);
            MPP_H26X_SYNTAXT_STRICT_CHECK(!(sps->chroma_format_idc > 3), "[sps] invalid chroma_format_idc", return false);
            if (sps->chroma_format_idc == H264ChromaFormat::MMP_H264_CHROMA_444)
            {
                br->U(1, sps->separate_colour_plane_flag);
                MPP_H26X_SYNTAXT_NORMAL_CHECK(sps->separate_colour_plane_flag, "[sps] separate_colour_plane_flag are not supported", return false);
            }
            br->UE(sps->bit_depth_luma_minus8);
            br->UE(sps->bit_depth_chroma_minus8);
            br->U(1, sps->qpprime_y_zero_transform_bypass_flag);
            br->U(1, sps->seq_scaling_matrix_present_flag);
            if (sps->seq_scaling_matrix_present_flag)
            {
                // TODO : UseDefaultScalingMatrix4x4Flag and UseDefaultScalingMatrix8x8Flag
                assert(false);
                int32_t loopTime = (sps->chroma_format_idc != H264ChromaFormat::MMP_H264_CHROMA_444) ? 8 : 12;
                sps->seq_scaling_list_present_flag.resize(loopTime);
                sps->ScalingList4x4.resize(6);
                sps->UseDefaultScalingMatrix4x4Flag.resize(6);
                sps->ScalingList8x8.resize(loopTime - 6);
                sps->UseDefaultScalingMatrix8x8Flag.resize(loopTime - 6);
                for (int32_t i=0; i<loopTime; i++)
                {
                    br->U(1, sps->seq_scaling_list_present_flag[i]);
                    if (sps->seq_scaling_list_present_flag[i])
                    {
                        if (i < 6)
                        {
                            if (!DeserializeScalingListSyntax(br, sps->ScalingList4x4[i], 16, sps->UseDefaultScalingMatrix4x4Flag[i]))
                            {
                                return false;
                            }
                            if (sps->UseDefaultScalingMatrix4x4Flag[i] == 1)
                            {
                                if (i < 3)
                                {
                                    sps->ScalingList4x4[i] = Default_4x4_Intra;
                                }
                                else
                                {
                                    sps->ScalingList4x4[i] = Default_4x4_Inter;
                                }
                            }
                        }
                        else
                        {
                            if (!DeserializeScalingListSyntax(br, sps->ScalingList8x8[i - 6], 64, sps->UseDefaultScalingMatrix8x8Flag[i]))
                            {
                                return false;
                            }
                            if (sps->UseDefaultScalingMatrix8x8Flag[i] == 1)
                            {
                                if (i % 2 == 0)
                                {
                                    sps->ScalingList8x8[i - 6] = Default_8x8_Intra;
                                }
                                else
                                {
                                    sps->ScalingList8x8[i - 6] = Default_8x8_Inter;
                                }
                            }
                        }
                    }
                    else
                    {
                        switch (i)
                        {
                            // 4x4
                            case 0:
                            {
                                sps->ScalingList4x4[i] = Default_4x4_Intra;
                                break;
                            }
                            case 3:
                            {
                                sps->ScalingList4x4[i] = Default_4x4_Inter;
                                break;
                            }
                            case 1:
                            case 2:
                            case 4:
                            case 5:
                            {
                                sps->ScalingList4x4[i] = sps->ScalingList4x4[i - 1];
                                break;
                            }
                            // 8x8
                            case 6:
                            {
                                sps->ScalingList8x8[i] = Default_8x8_Intra;
                                break;
                            }
                            case 7:
                            {
                                sps->ScalingList8x8[i] = Default_8x8_Inter;
                                break;
                            }
                            case 8:
                            case 9:
                            case 10:
                            case 11:
                            {
                                sps->ScalingList8x8[i] = sps->ScalingList8x8[i - 2];
                                break;
                            }
                            default:
                            {
                                assert(false);
                            }
                        }
                    }
                }
            }
            else
            {
                sps->chroma_format_idc = 1;
                sps->separate_colour_plane_flag = 0;
                sps->bit_depth_luma_minus8 = 0;
                sps->bit_depth_chroma_minus8 = 0;
                // (7-8)
                // Flat_4x4_16[ k ] = 16, with k = 0..15
                {
                    sps->ScalingList4x4.resize(6); /* 0..5 */
                    for (size_t i=0; i<6; i++)
                    {
                        sps->ScalingList4x4[i].resize(16);
                        for (size_t j=0; j<16; j++)
                        {
                            sps->ScalingList4x4[i][j] = 16;
                        }
                    }
                }
                // (7-9)
                // Flat_8x8_16[ k ] = 16, with k = 0..63
                {
                    sps->ScalingList8x8.resize(6); /* 6..11 */
                    for (size_t i=0; i<6; i++)
                    {
                        sps->ScalingList8x8[i].resize(64);
                        for (size_t j=0; j<64; j++)
                        {
                            sps->ScalingList8x8[i][j] = 16;
                        }
                    }
                }
            }
        }
        br->UE(sps->log2_max_frame_num_minus4);
        {
            // Hint : The value of log2_max_frame_num_minus4 shall be in the range of 0 to 12, inclusive.
            MPP_H26X_SYNTAXT_STRICT_CHECK(/* sps->log2_max_frame_num_minus4>=0 && */ sps->log2_max_frame_num_minus4<=12, "[sps] log2_max_frame_num_minus4 out of range", return false);
        }
        br->UE(sps->pic_order_cnt_type);
        {
            // Hint : The value of pic_order_cnt_type shall be in the range of 0 to 2, inclusive.
            MPP_H26X_SYNTAXT_STRICT_CHECK(/* sps->pic_order_cnt_type >= 0 && */ sps->pic_order_cnt_type <= 2, "[sps] pic_order_cnt_type out of range", return false);
        }
        if (sps->pic_order_cnt_type == 0)
        {
            br->UE(sps->log2_max_pic_order_cnt_lsb_minus4);
            // Hint : The value of log2_max_pic_order_cnt_lsb_minus4 shall be in the range of 0 to 12, inclusive.
            MPP_H26X_SYNTAXT_STRICT_CHECK(/* sps->log2_max_pic_order_cnt_lsb_minus4 >= 0 && */ sps->log2_max_pic_order_cnt_lsb_minus4 <= 12, "[sps] log2_max_pic_order_cnt_lsb_minus4 out of range", return false);
        }
        else if (sps->pic_order_cnt_type == 1)
        {
            br->U(1, sps->delta_pic_order_always_zero_flag);
            br->SE(sps->offset_for_non_ref_pic);
            br->SE(sps->offset_for_top_to_bottom_field);
            br->UE(sps->num_ref_frames_in_pic_order_cnt_cycle);
            sps->offset_for_ref_frame.resize(sps->num_ref_frames_in_pic_order_cnt_cycle + 1);
            for (uint32_t i=0; i<sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
            {
                br->SE(sps->offset_for_ref_frame[i]);
            }

        }
        br->UE(sps->max_num_ref_frames);
        br->U(1, sps->gaps_in_frame_num_value_allowed_flag);
        br->UE(sps->pic_width_in_mbs_minus1);
        br->UE(sps->pic_height_in_map_units_minus1);
        br->U(1, sps->frame_mbs_only_flag);
        if (!sps->frame_mbs_only_flag)
        {
            br->U(1, sps->mb_adaptive_frame_field_flag);
        }
        br->U(1, sps->direct_8x8_inference_flag);
        br->U(1, sps->frame_cropping_flag);
        if (sps->frame_cropping_flag)
        {
            br->UE(sps->frame_crop_left_offset);
            br->UE(sps->frame_crop_right_offset);
            br->UE(sps->frame_crop_top_offset);
            br->UE(sps->frame_crop_bottom_offset);
        }
        br->U(1, sps->vui_parameters_present_flag);
        if (sps->vui_parameters_present_flag)
        {
            sps->vui_seq_parameters = std::make_shared<H264VuiSyntax>();
            if (!DeserializeVuiSyntax(br, sps->vui_seq_parameters))
            {
                return false;
            }
        }
        br->rbsp_trailing_bits();
        FillH264SpsContext(sps);
        _contex->spsSet[sps->seq_parameter_set_id] = sps;
        _contex->sps = sps;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeSliceHeaderSyntax(H26xBinaryReader::ptr br, H264NalSyntax::ptr nal, H264SliceHeaderSyntax::ptr slice)
{
    // See aslo : ISO 14496/10(2020) - 7.3.3 Slice header syntax
    try
    {
        size_t begin = br->CurBits();
        H264PpsSyntax::ptr pps = nullptr;
        H264SpsSyntax::ptr sps = nullptr;
        bool IdrPicFlag = false;
        IdrPicFlag = nal->nal_unit_type == 5 /* MMP_H264_NALU_TYPE_IDR */ ? true : false;
        br->UE(slice->first_mb_in_slice);
        br->UE(slice->slice_type);
        slice->slice_type = slice->slice_type % 5; // See aslo : ISO 14496/10(2020) - Table 7-6 – Name association to slice_type 
        MPP_H26X_SYNTAXT_STRICT_CHECK(!(IdrPicFlag && slice->slice_type != H264SliceType::MMP_H264_I_SLICE), "[slice] A non-intra slice in an IDR NAL unit.", return false);
        br->UE(slice->pic_parameter_set_id);
        MPP_H26X_SYNTAXT_STRICT_CHECK(_contex->ppsSet.count(slice->pic_parameter_set_id), "[slice] missing pps", return false);
        pps = _contex->ppsSet[slice->pic_parameter_set_id];
        MPP_H26X_SYNTAXT_STRICT_CHECK(_contex->ppsSet.count(pps->seq_parameter_set_id), "[slice] missing sps", return false);
        sps = _contex->spsSet[pps->seq_parameter_set_id];
        if (sps->separate_colour_plane_flag == 1)
        {
            br->U(2, slice->colour_plane_id);
        }
        // Hint : frame_num is used as an identifier for pictures and shall be represented by log2_max_frame_num_minus4 + 4 bits in the bitstream.
        br->U(sps->log2_max_frame_num_minus4 + 4, slice->frame_num);
        if (!sps->frame_mbs_only_flag)
        {
            br->U(1, slice->field_pic_flag);
            if (slice->field_pic_flag)
            {
                br->U(1, slice->bottom_field_flag);
            }
        }
        if (IdrPicFlag)
        {
            br->UE(slice->idr_pic_id);
            {
                // Hint :
                // idr_pic_id identifies an IDR picture. The values of idr_pic_id in all the slices of an IDR picture shall remain unchanged. 
                // When two consecutive access units in decoding order are both IDR access units, the value of idr_pic_id in the slices of 
                // the first such IDR access unit shall differ from the idr_pic_id in the second such IDR access unit. The value of idr_pic_id 
                // shall be in the range of 0 to 65535, inclusive.
                MPP_H26X_SYNTAXT_STRICT_CHECK(/* slice->idr_pic_id >= 0 && */ slice->idr_pic_id <= 65535, "[slice] idr_pic_id out of range", return false);
            }
        }
        if (sps->pic_order_cnt_type == 0)
        {
            // Hint : The length of the pic_order_cnt_lsb syntax element is log2_max_pic_order_cnt_lsb_minus4 + 4 bits.
            br->U(sps->log2_max_pic_order_cnt_lsb_minus4 + 4, slice->pic_order_cnt_lsb);
            if (pps->bottom_field_pic_order_in_frame_present_flag && !slice->field_pic_flag)
            {
                br->SE(slice->delta_pic_order_cnt_bottom);
            }
        }
        if (sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flag)
        {
            br->SE(slice->delta_pic_order_cnt[0]);
            if (pps->bottom_field_pic_order_in_frame_present_flag && !slice->field_pic_flag)
            {
                br->SE(slice->delta_pic_order_cnt[1]);
            }
        }
        if (pps->redundant_pic_cnt_present_flag)
        {
            br->SE(slice->redundant_pic_cnt);
        }
        if (slice->slice_type == H264SliceType::MMP_H264_B_SLICE)
        {
            br->U(1, slice->direct_spatial_mv_pred_flag);
        }
        if (slice->slice_type == H264SliceType::MMP_H264_P_SLICE || slice->slice_type == H264SliceType::MMP_H264_SP_SLICE || slice->slice_type == H264SliceType::MMP_H264_B_SLICE)
        {
            br->U(1, slice->num_ref_idx_active_override_flag);
            if (slice->num_ref_idx_active_override_flag)
            {
                br->UE(slice->num_ref_idx_l0_active_minus1);
                if (slice->slice_type == H264SliceType::MMP_H264_B_SLICE)
                {
                    br->UE(slice->num_ref_idx_l1_active_minus1);
                }
            }
            else /* if (slice->num_ref_idx_active_override_flag == 0) */
            {
                slice->num_ref_idx_l0_active_minus1 = pps->num_ref_idx_l0_default_active_minus1;
                if (slice->slice_type == H264SliceType::MMP_H264_B_SLICE)
                {
                    slice->num_ref_idx_l1_active_minus1 = pps->num_ref_idx_l1_default_active_minus1;
                }
            }
        }
        if (nal->nal_unit_type == 20 /* MMP_H264_NALU_TYPE_SLC_EXT */ || nal->nal_unit_type == 21)
        {
            MPP_H26X_SYNTAXT_STRICT_CHECK(false, "[slice] not support ref_pic_list_mvc_modification() feature", return false);
        }
        else
        {
            slice->rplm = std::make_shared<H264ReferencePictureListModificationSyntax>();
            if (!DeserializeReferencePictureListModificationSyntax(br, slice, slice->rplm))
            {
                return false;
            }
        }
        if ((pps->weighted_pred_flag && (slice->slice_type == H264SliceType::MMP_H264_P_SLICE || slice->slice_type == H264SliceType::MMP_H264_SP_SLICE)) ||
            (pps->weighted_bipred_idc == 1 && slice->slice_type == H264SliceType::MMP_H264_B_SLICE)
        )
        {
            slice->pwt = std::make_shared<H264PredictionWeightTableSyntax>();
            if (!DeserializePredictionWeightTableSyntax(br, sps, slice, slice->pwt))
            {
                return false;
            }
        }
        if (nal->nal_ref_idc != 0)
        {
            slice->drpm = std::make_shared<H264DecodedReferencePictureMarkingSyntax>();
            if (!DeserializeDecodedReferencePictureMarkingSyntax(br, nal, slice->drpm))
            {
                return false;
            }
        }
        if (pps->entropy_coding_mode_flag && slice->slice_type != H264SliceType::MMP_H264_I_SLICE && slice->slice_type != H264SliceType::MMP_H264_SI_SLICE)
        {
            br->UE(slice->cabac_init_idc);
            {
                // Hint :  The value of cabac_init_idc shall be in the range of 0 to 2, inclusive.
                // WORKAROUND : cabac_init_idc may be other value, for example 7 ???
                MPP_H26X_SYNTAXT_NORMAL_CHECK(/* slice->cabac_init_idc >= 0 && */ slice->cabac_init_idc <= 2, "[slice] cabac_init_idc out of range", ;);
            }
        }
        br->SE(slice->slice_qp_delta);
        if (slice->slice_type == H264SliceType::MMP_H264_SP_SLICE || slice->slice_type == H264SliceType::MMP_H264_SI_SLICE)
        {
            if (slice->slice_type == H264SliceType::MMP_H264_SP_SLICE)
            {
                br->U(1, slice->sp_for_switch_flag);
            }
            br->SE(slice->slice_qs_delta);
        }
        if (pps->deblocking_filter_control_present_flag)
        {
            br->UE(slice->disable_deblocking_filter_idc);
            if (slice->disable_deblocking_filter_idc != 1)
            {
                br->SE(slice->slice_alpha_c0_offset_div2);
                br->SE(slice->slice_beta_offset_div2);
            }
        }
        if (pps->num_slice_groups_minus1>0 &&
            pps->slice_group_map_type>=3 && pps->slice_group_map_type<=5
        )
        {
            br->U(2, slice->slice_group_change_cycle);
        }
        slice->slice_data_bit_offset = (uint16_t)(br->CurBits() - begin);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeDecodedReferencePictureMarkingSyntax(H26xBinaryReader::ptr br, H264NalSyntax::ptr nal, H264DecodedReferencePictureMarkingSyntax::ptr drpm)
{
    // See also : ISO 14496/10(2020) - 7.3.3.3 Decoded reference picture marking syntax
    try
    {
        bool IdrPicFlag = nal->nal_unit_type == 5 /* MMP_H264_NALU_TYPE_IDR */ ? true : false;
        if (IdrPicFlag)
        {
            br->U(1, drpm->no_output_of_prior_pics_flag);
            br->U(1, drpm->long_term_reference_flag);
        }
        else
        {
            // See also : ISO 14496/10(2020) - Table 7-9 – Memory management control operation (memory_management_control_operation) values
            br->U(1, drpm->adaptive_ref_pic_marking_mode_flag);
            if (drpm->adaptive_ref_pic_marking_mode_flag)
            {
                uint32_t memory_management_control_operation = 0;
                do
                {
                    br->UE(memory_management_control_operation);
                    if (memory_management_control_operation == 1 ||
                        memory_management_control_operation == 3
                    )
                    {
                        H264DecodedReferencePictureMarkingSyntax::memory_management_control_operations_data data;
                        br->UE(data.difference_of_pic_nums_minus1);
                        drpm->memory_management_control_operations_datas.push_back(data);
                    }
                    if (memory_management_control_operation == 2)
                    {
                        H264DecodedReferencePictureMarkingSyntax::memory_management_control_operations_data data;
                        br->UE(data.long_term_pic_num);
                        drpm->memory_management_control_operations_datas.push_back(data);
                    }
                    if (memory_management_control_operation == 3 ||
                            memory_management_control_operation == 6
                    )
                    {
                        H264DecodedReferencePictureMarkingSyntax::memory_management_control_operations_data data;
                        br->UE(data.long_term_frame_idx);
                        drpm->memory_management_control_operations_datas.push_back(data);
                    }
                    if (memory_management_control_operation == 4)
                    {
                        H264DecodedReferencePictureMarkingSyntax::memory_management_control_operations_data data;
                        br->UE(data.max_long_term_frame_idx_plus1);
                        drpm->memory_management_control_operations_datas.push_back(data);
                    }
                    drpm->memory_management_control_operations.push_back(memory_management_control_operation);
                } while (memory_management_control_operation != 0);
            }
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeSubSpsSyntax(H26xBinaryReader::ptr br, H264SpsSyntax::ptr sps, H264SubSpsSyntax::ptr subSps)
{
    // See also : ISO 14496/10(2020) - 7.3.2.1.3 Subset sequence parameter set RBSP syntax
    try
    {
        if (!DeserializeSpsSyntax(br, sps))
        {
            return false;
        }
        if (sps->profile_idc == 83 || sps->profile_idc == 86)
        {
            // TODO
            // See aslo : ISO 14496/10(2020) - F.3.3.2.1.4 Sequence parameter set SVC extension syntax
            assert(false);
            return false;
        }
        else if (sps->profile_idc == 118 || sps->profile_idc == 128 ||
            sps->profile_idc == 134
        )
        {
            br->U(1, subSps->bit_equal_to_one);
            subSps->mvc = std::make_shared<H264SpsMvcSyntax>();
            if (!DeserializeSpsMvcSyntax(br, subSps->mvc))
            {
                return false;
            }
            br->U(1, subSps->mvc_vui_parameters_present_flag);
            if (subSps->mvc_vui_parameters_present_flag)
            {
                subSps->mvcVui = std::make_shared<H264MvcVuiSyntax>();
                if (!DeserializeMvcVuiSyntax(br, subSps->mvcVui))
                {
                    return false;   
                }
            }
            assert(false);
            return false;
        }
        else if (sps->profile_idc == 138 || sps->profile_idc == 135)
        {
            // TODO
            // See aslo : ISO 14496/10(2020) - H.3.3.2.1.5 Sequence parameter set MVCD extension syntax
            assert(false);
            return false;
        }
        else if (sps->profile_idc == 139)
        {
            // TODO
            // See aslo : ISO 14496/10(2020) - H.3.3.2.1.5 Sequence parameter set MVCD extension syntax
            //                                 I.3.3.2.1.5 Sequence parameter set MVCD extension syntax
            assert(false);
            return false;
        }
        br->U(1, subSps->additional_extension2_flag);
        // TODO
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeSpsMvcSyntax(H26xBinaryReader::ptr br, H264SpsMvcSyntax::ptr mvc)
{
    // See also : ISO 14496/10(2020) - G.3.3.2.1.4 Sequence parameter set MVC extension syntax
    try
    {
        br->UE(mvc->num_views_minus1);
        mvc->view_id.resize(mvc->num_views_minus1 + 1);
        for (uint32_t i=0; i<=mvc->num_views_minus1; i++)
        {
            br->UE(mvc->view_id[i]);
        }
        mvc->num_anchor_refs_l0.resize(mvc->num_views_minus1 + 1);
        mvc->anchor_ref_l0.resize(mvc->num_views_minus1 + 1);
        mvc->num_anchor_refs_l1.resize(mvc->num_views_minus1 + 1);
        mvc->anchor_ref_l1.resize(mvc->num_views_minus1 + 1);
        for (uint32_t i=1; i<=mvc->num_views_minus1; i++)
        {
            br->UE(mvc->num_anchor_refs_l0[i]);
            mvc->anchor_ref_l0[i].resize(mvc->num_anchor_refs_l0[i] + 1);
            for (uint32_t j=0; j<mvc->num_anchor_refs_l0[i]; j++)
            {
                br->UE(mvc->anchor_ref_l0[i][j]);
            }
            br->UE(mvc->num_anchor_refs_l1[i]);
            mvc->anchor_ref_l1[i].resize(mvc->num_anchor_refs_l1[i] + 1);
            for (int j=0; i<mvc->num_anchor_refs_l1[i]; j++)
            {
                br->UE(mvc->anchor_ref_l1[i][j]);
            }
        }
        mvc->num_non_anchor_refs_l0.resize(mvc->num_views_minus1 + 1);
        mvc->non_anchor_ref_l0.resize(mvc->num_views_minus1 + 1);
        mvc->num_non_anchor_refs_l1.resize(mvc->num_views_minus1 + 1);
        mvc->non_anchor_ref_l1.resize(mvc->num_views_minus1 + 1);
        for (uint32_t i=0; i<=mvc->num_views_minus1; i++)
        {
            br->UE(mvc->num_non_anchor_refs_l0[i]);
            mvc->non_anchor_ref_l0[i].resize(mvc->num_non_anchor_refs_l0[i] + 1);
            for (uint32_t j=0; j<mvc->num_non_anchor_refs_l0[i]; j++)
            {
                br->UE(mvc->non_anchor_ref_l0[i][j]);
            }
            br->UE(mvc->num_non_anchor_refs_l1[i]);
            mvc->non_anchor_ref_l1[i].resize(mvc->num_non_anchor_refs_l1[i] + 1);
            for (int j=0; i<mvc->num_non_anchor_refs_l1[i]; j++)
            {
                br->UE(mvc->non_anchor_ref_l1[i][j]);
            }
        }
        br->UE(mvc->num_level_values_signalled_minus1);
        mvc->level_idc.resize(mvc->num_level_values_signalled_minus1 + 1);
        mvc->num_applicable_ops_minus1.resize(mvc->num_level_values_signalled_minus1 + 1);
        mvc->applicable_op_temporal_id.resize(mvc->num_level_values_signalled_minus1 + 1);
        mvc->applicable_op_num_target_views_minus1.resize(mvc->num_level_values_signalled_minus1 + 1);
        mvc->applicable_op_target_view_id.resize(mvc->num_level_values_signalled_minus1 + 1);
        mvc->applicable_op_num_views_minus1.resize(mvc->num_level_values_signalled_minus1 + 1);
        for (uint32_t i=0; i<=mvc->num_level_values_signalled_minus1; i++)
        {
            br->U(8, mvc->level_idc[i]);
            br->UE(mvc->num_applicable_ops_minus1[i]);
            mvc->applicable_op_temporal_id[i].resize(mvc->num_applicable_ops_minus1[i] + 1);
            mvc->applicable_op_num_target_views_minus1[i].resize(mvc->num_applicable_ops_minus1[i] + 1);
            mvc->applicable_op_target_view_id[i].resize(mvc->num_applicable_ops_minus1[i] + 1);
            mvc->applicable_op_num_views_minus1[i].resize(mvc->num_applicable_ops_minus1[i] + 1);
            for (uint32_t j=0; j<=mvc->num_applicable_ops_minus1[i]; j++)
            {
                br->U(3, mvc->applicable_op_temporal_id[i][j]);
                br->UE(mvc->applicable_op_num_target_views_minus1[i][j]);
                mvc->applicable_op_target_view_id[i][j].resize(mvc->applicable_op_num_target_views_minus1[i][j] + 1);
                for (uint32_t k=0; k<=mvc->applicable_op_num_target_views_minus1[i][j]; k++)
                {
                    br->UE(mvc->applicable_op_target_view_id[i][j][k]);
                }
                br->UE(mvc->applicable_op_num_views_minus1[i][j]);
            }
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
    
}

bool H264Deserialize::DeserializeMvcVuiSyntax(H26xBinaryReader::ptr br, H264MvcVuiSyntax::ptr mvcVui)
{
    // See also : ISO 14496/10(2020) - G.10.1 MVC VUI parameters extension syntax
    try
    {
        br->UE(mvcVui->vui_mvc_num_ops_minus1);
        mvcVui->vui_mvc_temporal_id.resize(mvcVui->vui_mvc_num_ops_minus1 + 1);
        mvcVui->vui_mvc_num_target_output_views_minus1.resize(mvcVui->vui_mvc_num_ops_minus1 + 1);
        mvcVui->vui_mvc_view_id.resize(mvcVui->vui_mvc_num_ops_minus1 + 1);
        mvcVui->vui_mvc_timing_info_present_flag.resize(mvcVui->vui_mvc_num_ops_minus1 + 1);
        mvcVui->vui_mvc_num_units_in_tick.resize(mvcVui->vui_mvc_num_ops_minus1 + 1);
        mvcVui->vui_mvc_time_scale.resize(mvcVui->vui_mvc_num_ops_minus1 + 1);
        mvcVui->vui_mvc_fixed_frame_rate_flag.resize(mvcVui->vui_mvc_num_ops_minus1 + 1);
        mvcVui->vui_mvc_nal_hrd_parameters_present_flag.resize(mvcVui->vui_mvc_num_ops_minus1 + 1);
        mvcVui->nalHrds.resize(mvcVui->vui_mvc_num_ops_minus1 + 1);
        mvcVui->vui_mvc_vcl_hrd_parameters_present_flag.resize(mvcVui->vui_mvc_num_ops_minus1 + 1);
        mvcVui->vclHrds.resize(mvcVui->vui_mvc_num_ops_minus1 + 1);
        mvcVui->vui_mvc_low_delay_hrd_flag.resize(mvcVui->vui_mvc_num_ops_minus1 + 1);
        mvcVui->vui_mvc_pic_struct_present_flag.resize(mvcVui->vui_mvc_num_ops_minus1 + 1);
        for (uint32_t i=0; i<=mvcVui->vui_mvc_num_ops_minus1; i++)
        {
            br->U(3, mvcVui->vui_mvc_temporal_id[i]);
            br->UE(mvcVui->vui_mvc_num_target_output_views_minus1[i]);
            mvcVui->vui_mvc_view_id[i].resize(mvcVui->vui_mvc_num_target_output_views_minus1[i] + 1);
            for (uint32_t j=0; j<=mvcVui->vui_mvc_num_target_output_views_minus1[i]; j++)
            {
                br->UE(mvcVui->vui_mvc_view_id[i][j]);
            }
            br->U(1, mvcVui->vui_mvc_timing_info_present_flag[i]);
            if (mvcVui->vui_mvc_timing_info_present_flag[i])
            {
                br->U(32, mvcVui->vui_mvc_num_units_in_tick[i]);
                br->U(32, mvcVui->vui_mvc_time_scale[i]);
                br->U(1, mvcVui->vui_mvc_fixed_frame_rate_flag[i]);
            }
            br->U(1, mvcVui->vui_mvc_nal_hrd_parameters_present_flag[i]);
            if (mvcVui->vui_mvc_nal_hrd_parameters_present_flag[i])
            {
                mvcVui->nalHrds[i] = std::make_shared<H264HrdSyntax>();
                if (!DeserializeHrdSyntax(br, mvcVui->nalHrds[i]))
                {
                    return false;
                }
            }
            br->U(1, mvcVui->vui_mvc_vcl_hrd_parameters_present_flag[i]);
            if (mvcVui->vui_mvc_vcl_hrd_parameters_present_flag[i])
            {
                mvcVui->vclHrds[i] = std::make_shared<H264HrdSyntax>();
                if (!DeserializeHrdSyntax(br, mvcVui->vclHrds[i]))
                {
                    return false;
                }
            }
            if (mvcVui->vui_mvc_nal_hrd_parameters_present_flag[i] ||
                mvcVui->vui_mvc_vcl_hrd_parameters_present_flag[i]
            )
            {
                br->U(1, mvcVui->vui_mvc_low_delay_hrd_flag[i]);
            }
            br->U(1, mvcVui->vui_mvc_pic_struct_present_flag[i]);
        }
        return true;
    }
    catch(const std::exception& /* e */)
    {
        return false;
    }
}

bool H264Deserialize::DeserializePpsSyntax(H26xBinaryReader::ptr br, H264PpsSyntax::ptr pps)
{
    // See aslo : ISO 14496/10(2020) - 7.3.2.2 Picture parameter set RBSP syntax
    try
    {
        br->more_rbsp_data();
        br->UE(pps->pic_parameter_set_id);
        MPP_H26X_SYNTAXT_STRICT_CHECK(pps->pic_parameter_set_id >= 0 && pps->pic_parameter_set_id <= 255, "[sps] pic_parameter_set_id out of range", return false);
        br->UE(pps->seq_parameter_set_id);
        MPP_H26X_SYNTAXT_STRICT_CHECK(pps->seq_parameter_set_id >= 0 && pps->seq_parameter_set_id <= 32, "[sps] seq_parameter_set_id out of range", return false);
        if (_contex->spsSet.count(pps->seq_parameter_set_id) == 0)
        {
            assert(false);
            return false;
        }
        H264SpsSyntax::ptr sps = _contex->spsSet[pps->seq_parameter_set_id];
        br->U(1, pps->entropy_coding_mode_flag);
        br->U(1, pps->bottom_field_pic_order_in_frame_present_flag);
        br->UE(pps->num_slice_groups_minus1);
        if (pps->num_slice_groups_minus1)
        {
            br->UE(pps->slice_group_map_type);
            // Reference : FFmpeg 6.x
            MPP_H26X_SYNTAXT_STRICT_CHECK(pps->slice_group_map_type > 0, "[sps] not support slice_group_map_type, missing feature", return false);
        }
        br->UE(pps->num_ref_idx_l0_default_active_minus1);
        br->UE(pps->num_ref_idx_l1_default_active_minus1);
        {
            // Hint :
            // num_ref_idx_l0_default_active_minus1
            //   num_ref_idx_l0_default_active_minus1 specifies how num_ref_idx_l0_active_minus1 is inferred for P, SP, and B 
            // slices with num_ref_idx_active_override_flag equal to 0. The value of num_ref_idx_l0_default_active_minus1 shall be 
            // in the range of 0 to 31, inclusive.
            // num_ref_idx_l1_default_active_minus1
            //   num_ref_idx_l1_default_active_minus1 specifies how num_ref_idx_l1_active_minus1 is inferred for B slices with 
            // num_ref_idx_active_override_flag equal to 0. The value of num_ref_idx_l1_default_active_minus1 shall be in the range    
            // of 0 to 31, inclusive.
            // 
            MPP_H26X_SYNTAXT_STRICT_CHECK(pps->num_ref_idx_l0_default_active_minus1 >= 0 && pps->num_ref_idx_l0_default_active_minus1 <= 31, "[sps] num_ref_idx_l0_default_active_minus1 out of range", return false);
            MPP_H26X_SYNTAXT_STRICT_CHECK(pps->num_ref_idx_l1_default_active_minus1 >= 0 && pps->num_ref_idx_l1_default_active_minus1 <= 31, "[sps] num_ref_idx_active_override_flag out of range", return false);
        }
        br->U(1, pps->weighted_pred_flag);
        br->U(2, pps->weighted_bipred_idc);
        br->SE(pps->pic_init_qp_minus26);
        br->SE(pps->pic_init_qs_minus26);
        br->SE(pps->chroma_qp_index_offset);
        {
            // Hint : chroma_qp_index_offset specifies the offset that shall be added to QPY and QSY for addressing the table of QPC values 
            // for the Cb chroma component. The value of chroma_qp_index_offset shall be in the range of −12 to +12, inclusive.
            MPP_H26X_SYNTAXT_STRICT_CHECK(pps->chroma_qp_index_offset >= -12 && pps->chroma_qp_index_offset <= 12, "[sps] chroma_qp_index_offset out of range", return false);
        }
        br->U(1, pps->deblocking_filter_control_present_flag);
        br->U(1, pps->constrained_intra_pred_flag);
        br->U(1, pps->redundant_pic_cnt_present_flag);
        if (br->more_rbsp_data())
        {
            br->U(1, pps->transform_8x8_mode_flag);
            br->U(1, pps->pic_scaling_matrix_present_flag);
            if (pps->pic_scaling_matrix_present_flag)
            {
                int32_t loopTime = ((sps->chroma_format_idc != H264ChromaFormat::MMP_H264_CHROMA_444) ? 2 : 6) * pps->transform_8x8_mode_flag;
                pps->pic_scaling_list_present_flag.resize(loopTime);
                pps->ScalingList4x4.resize(6);
                pps->UseDefaultScalingMatrix4x4Flag.resize(6);
                pps->ScalingList8x8.resize(loopTime - 6);
                pps->UseDefaultScalingMatrix8x8Flag.resize(loopTime - 6);
                for (int32_t i=0; i<loopTime; i++)
                {
                    br->U(1, pps->pic_scaling_list_present_flag[i]);
                    if (pps->pic_scaling_list_present_flag[i])
                    {
                        if (i < 6)
                        {
                            if (!DeserializeScalingListSyntax(br, pps->ScalingList4x4[i], 16, pps->UseDefaultScalingMatrix4x4Flag[i]))
                            {
                                return false;
                            }
                            if (pps->UseDefaultScalingMatrix4x4Flag[i] == 1)
                            {
                                // TODO
                            }
                        }
                        else
                        {
                            if (!DeserializeScalingListSyntax(br, pps->ScalingList8x8[i - 6], 64, pps->UseDefaultScalingMatrix8x8Flag[i]))
                            {
                                return false;
                            }
                            if (pps->UseDefaultScalingMatrix8x8Flag[i] == 1)
                            {
                                // TODO
                            }
                        }
                    }
                }
                br->SE(pps->second_chroma_qp_index_offset);
                {
                    // Hint : second_chroma_qp_index_offset specifies the offset that shall be added to QPY and QSY for addressing the table of 
                    // QPC values for the Cr chroma component. The value of second_chroma_qp_index_offset shall be in the range of −12 to 
                    // +12, inclusive.
                    MPP_H26X_SYNTAXT_STRICT_CHECK(pps->second_chroma_qp_index_offset >= -12 && pps->second_chroma_qp_index_offset <= 12, "[sps] second_chroma_qp_index_offset out of range", return false);
                }
            }
            else
            {
                // Hint : When second_chroma_qp_index_offset is not present, it shall be inferred to be equal to chroma_qp_index_offset
                pps->second_chroma_qp_index_offset = pps->chroma_qp_index_offset;
            }
        }
        if (!pps->pic_scaling_matrix_present_flag)
        {
            pps->ScalingList4x4 = sps->ScalingList4x4;
            pps->ScalingList8x8 = sps->ScalingList8x8;
        }
        br->rbsp_trailing_bits();
        _contex->ppsSet[pps->pic_parameter_set_id] = pps;
        _contex->pps = pps;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeNalSvcSyntax(H26xBinaryReader::ptr br, H264NalSvcSyntax::ptr svc)
{
    // See also : ISO 14496/10(2020) - F.3.3.1.1 NAL unit header SVC extension syntax
    try
    {
        br->U(1, svc->idr_flag);
        br->U(6, svc->priority_id);
        br->U(1, svc->no_inter_layer_pred_flag);
        br->U(3, svc->dependency_id);
        br->U(4, svc->quality_id);
        br->U(3, svc->temporal_id);
        br->U(1, svc->use_ref_base_pic_flag);
        br->U(1, svc->discardable_flag);
        br->U(1, svc->output_flag);
        br->U(1, svc->reserved_three_2bits);
        return true;
    }
    catch(const std::exception& /* e */)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeNal3dAvcSyntax(H26xBinaryReader::ptr br, H264Nal3dAvcSyntax::ptr avc)
{
    // See also : ISO 14496/10(2020) - I.3.3.1.1 NAL unit header 3D-AVC extension syntax
    try
    {
        br->U(8, avc->view_idx);
        br->U(1, avc->depth_flag);
        br->U(1, avc->non_idr_flag);
        br->U(3, avc->temporal_id);
        br->U(1, avc->anchor_pic_flag);
        br->U(1, avc->inter_view_flag);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeNalMvcSyntax(H26xBinaryReader::ptr br, H264NalMvcSyntax::ptr mvc)
{
    // See also : ISO 14496/10(2020) - I.3.3.1.1 NAL unit header 3D-AVC extension syntax
    try
    {
        br->U(1, mvc->non_idr_flag);
        br->U(6, mvc->priority_id);
        br->U(10, mvc->view_id);
        br->U(1, mvc->temporal_id);
        br->U(1, mvc->anchor_pic_flag);
        br->U(1, mvc->inter_view_flag);
        br->U(1, mvc->reserved_one_bit);
        return true;
    }
    catch (...)
    {
        return false;
    }
    
}

bool H264Deserialize::DeserializeScalingListSyntax(H26xBinaryReader::ptr br, std::vector<int32_t>& scalingList, int32_t sizeOfScalingList, int32_t& useDefaultScalingMatrixFlag)
{
    // See aslo : ISO 14496/10(2020) - 7.3.2.1.1.1 Scaling list syntax
    try
    {
        int32_t lastScale = 8;
        int32_t nextScale = 8;
        int32_t delta_scale = 0;
        scalingList.resize(sizeOfScalingList);
        for (int32_t j=0; j<sizeOfScalingList; j++)
        {
            if (nextScale != 0)
            {
                br->SE(delta_scale);
                nextScale = (lastScale + delta_scale + 256) % 256;
                useDefaultScalingMatrixFlag = (j == 0 && nextScale == 0);
            }
            scalingList[j] = (nextScale == 0) ? lastScale : nextScale;
            lastScale = scalingList[j];
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeReferencePictureListModificationSyntax(H26xBinaryReader::ptr br, H264SliceHeaderSyntax::ptr slice, H264ReferencePictureListModificationSyntax::ptr rplm)
{
    // See also : ISO 14496/10(2020) - 7.3.3.1 Reference picture list modification syntax
    try
    {
        if (slice->slice_type != 2 /* MMP_H264_I_SLICE */ && slice->slice_type != 4 /* MMP_H264_SI_SLICE */)
        {
            br->U(1, rplm->ref_pic_list_modification_flag_l0);
            if (rplm->ref_pic_list_modification_flag_l0)
            {
                uint32_t  modification_of_pic_nums_idc = 0;
                do
                {
                    br->UE(modification_of_pic_nums_idc);
                    if (modification_of_pic_nums_idc == 0 ||
                        modification_of_pic_nums_idc == 1
                    )
                    {
                        H264ReferencePictureListModificationSyntax::modification_of_pic_nums_idcs_data modification_of_pic_nums_idcs_data;
                        br->UE(modification_of_pic_nums_idcs_data.abs_diff_pic_num_minus1);
                        rplm->modification_of_pic_nums_idcs_datas.push_back(modification_of_pic_nums_idcs_data);
                    }
                    else if (modification_of_pic_nums_idc == 2)
                    {
                        H264ReferencePictureListModificationSyntax::modification_of_pic_nums_idcs_data modification_of_pic_nums_idcs_data;
                        br->UE(modification_of_pic_nums_idcs_data.long_term_pic_num);
                        rplm->modification_of_pic_nums_idcs_datas.push_back(modification_of_pic_nums_idcs_data);
                    }
                    rplm->modification_of_pic_nums_idcs.push_back(modification_of_pic_nums_idc);
                } while(modification_of_pic_nums_idc != 3);
            }
        }
        if (slice->slice_type == 1 /* MMP_H264_B_SLICE */)
        {
            br->U(1, rplm->ref_pic_list_modification_flag_l1);
            if (rplm->ref_pic_list_modification_flag_l1)
            {
                uint32_t  modification_of_pic_nums_idc = 0;
                do
                {
                    br->UE(modification_of_pic_nums_idc);
                    if (modification_of_pic_nums_idc == 0 ||
                        modification_of_pic_nums_idc == 1
                    )
                    {
                        H264ReferencePictureListModificationSyntax::modification_of_pic_nums_idcs_data modification_of_pic_nums_idcs_data;
                        br->UE(modification_of_pic_nums_idcs_data.abs_diff_pic_num_minus1);
                        rplm->modification_of_pic_nums_idcs_datas.push_back(modification_of_pic_nums_idcs_data);
                    }
                    else if (modification_of_pic_nums_idc == 2)
                    {
                        H264ReferencePictureListModificationSyntax::modification_of_pic_nums_idcs_data modification_of_pic_nums_idcs_data;
                        br->UE(modification_of_pic_nums_idcs_data.long_term_pic_num);
                        rplm->modification_of_pic_nums_idcs_datas.push_back(modification_of_pic_nums_idcs_data);
                    }
                    rplm->modification_of_pic_nums_idcs.push_back(modification_of_pic_nums_idc);
                } while(modification_of_pic_nums_idc != 3);
            }
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializePredictionWeightTableSyntax(H26xBinaryReader::ptr br, H264SpsSyntax::ptr sps, H264SliceHeaderSyntax::ptr slice, H264PredictionWeightTableSyntax::ptr pwt)
{
    // See also : ISO 14496/10(2020) - 7.3.3.2 Prediction weight table syntax
    try
    {
        uint32_t ChromaArrayType = sps->separate_colour_plane_flag == 1 ? 0 : sps->chroma_format_idc;
        br->UE(pwt->luma_log2_weight_denom);
        {
            // Hint : luma_log2_weight_denom is the base 2 logarithm of the denominator for all luma weighting factors. The value of 
            // luma_log2_weight_denom shall be in the range of 0 to 7, inclusive.
            MPP_H26X_SYNTAXT_STRICT_CHECK(pwt->luma_log2_weight_denom >= 0 && pwt->luma_log2_weight_denom <= 7, "[pwt] luma_log2_weight_denom out of range", return false);
        }
        if (ChromaArrayType != 0)
        {
            br->UE(pwt->chroma_log2_weight_denom);
            {
                // Hint : chroma_log2_weight_denom is the base 2 logarithm of the denominator for all chroma weighting factors. The value of 
                // chroma_log2_weight_denom shall be in the range of 0 to 7, inclusive.
                MPP_H26X_SYNTAXT_STRICT_CHECK(pwt->chroma_log2_weight_denom >= 0 && pwt->chroma_log2_weight_denom <= 7, "[pwt] chroma_log2_weight_denom out of range", return false);
            }
        }
        pwt->luma_weight_l0_flag.resize(slice->num_ref_idx_l0_active_minus1 + 1);
        pwt->luma_weight_l0.resize(slice->num_ref_idx_l0_active_minus1 + 1);
        pwt->luma_offset_l0.resize(slice->num_ref_idx_l0_active_minus1 + 1);
        pwt->chroma_weight_l0_flag.resize(slice->num_ref_idx_l0_active_minus1 + 1);
        pwt->chroma_weight_l0.resize(slice->num_ref_idx_l0_active_minus1 + 1);
        pwt->chroma_offset_l0.resize(slice->num_ref_idx_l0_active_minus1 + 1);
        for (uint32_t i=0; i<=slice->num_ref_idx_l0_active_minus1; i++)
        {
            br->U(1, pwt->luma_weight_l0_flag[i]);
            if (pwt->luma_weight_l0_flag[i])
            {
                br->SE(pwt->luma_weight_l0[i]);
                br->SE(pwt->luma_offset_l0[i]);
            }
            else
            {
                // Hint : When luma_weight_l0_flag is equal to 0, luma_weight_l0[ i ] shall be inferred to be equal to 2^luma_log2_weight_denom for RefPicList0[ i ].
                pwt->luma_weight_l0[i] = 1 << pwt->luma_log2_weight_denom;
            }
            if (ChromaArrayType != 0)
            {
                br->U(1, pwt->chroma_weight_l0_flag[i]);
                if (pwt->chroma_weight_l0_flag[i])
                {
                    pwt->chroma_weight_l0[i].resize(2);
                    pwt->chroma_offset_l0[i].resize(2);
                    for (size_t j=0; j<2; j++)
                    {
                        br->SE(pwt->chroma_weight_l0[i][j]);
                        br->SE(pwt->chroma_offset_l0[i][j]);
                    }
                }
            }
        }
        if (slice->slice_type == 1 /* MMP_H264_B_SLICE */)
        {
            pwt->luma_weight_l1_flag.resize(slice->num_ref_idx_l1_active_minus1 + 1);
            pwt->luma_weight_l1.resize(slice->num_ref_idx_l1_active_minus1 + 1);
            pwt->luma_offset_l1.resize(slice->num_ref_idx_l1_active_minus1 + 1);
            pwt->chroma_weight_l1_flag.resize(slice->num_ref_idx_l1_active_minus1 + 1);
            pwt->chroma_weight_l1.resize(slice->num_ref_idx_l1_active_minus1 + 1);
            pwt->chroma_offset_l1.resize(slice->num_ref_idx_l1_active_minus1 + 1);
            for (uint32_t i=0; i<=slice->num_ref_idx_l1_active_minus1; i++)
            {
                br->U(1, pwt->luma_weight_l1_flag[i]);
                if (pwt->luma_weight_l1_flag[i])
                {
                    br->SE(pwt->luma_weight_l1[i]);
                    br->SE(pwt->luma_weight_l1[i]);
                }
                else
                {
                    pwt->luma_weight_l1[i] = 1 << pwt->luma_log2_weight_denom;
                }
                if (ChromaArrayType != 0)
                {
                    br->U(1, pwt->chroma_weight_l1_flag[i]);
                    if (pwt->chroma_weight_l1_flag[i])
                    {
                        pwt->chroma_weight_l1[i].resize(2);
                        pwt->chroma_offset_l1[i].resize(2);
                        for (size_t j=0; j<2; j++)
                        {
                            br->SE(pwt->chroma_weight_l1[i][j]);
                            br->SE(pwt->chroma_offset_l1[i][j]);
                        }
                    }
                    else
                    {
                        for (size_t j=0; j<2; j++)
                        {
                            pwt->chroma_weight_l1[i][j] = 1 << pwt->chroma_log2_weight_denom;
                        }
                    }
                }
            }
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeSeiBufferPeriodSyntax(H26xBinaryReader::ptr br, H264SeiBufferPeriodSyntax::ptr bp)
{
    // See also : ISO 14496/10(2020) - D.1.2 Buffering period SEI message syntax
    try
    {
        br->UE(bp->seq_parameter_set_id);

        if (_contex->spsSet.count(bp->seq_parameter_set_id) == 0)
        {
            assert(false);
            return false;
        }
        H264SpsSyntax::ptr sps = _contex->spsSet[bp->seq_parameter_set_id];

        // The variable NalHrdBpPresentFlag is derived as follows:
        // - If any of the following is true, the value of NalHrdBpPresentFlag shall be set equal to 1:
        //  - nal_hrd_parameters_present_flag is present in the bitstream and is equal to 1,
        //  - the need for the presence of buffering period parameters for NAL HRD operation in the bitstream in buffering 
        //    period SEI messages is determined by the application, by some means not specified in this Recommendation | 
        //    International Standard.
        // - Otherwise, the value of NalHrdBpPresentFlag shall be set equal to 0
        // 
        // The variable VclHrdBpPresentFlag is derived as follows:
        // - If any of the following is true, the value of VclHrdBpPresentFlag shall be set equal to 1:
        //  - vcl_hrd_parameters_present_flag is present in the bitstream and is equal to 1,
        //  - the need for the presence of buffering period parameters for VCL HRD operation in the bitstream in buffering 
        //    period SEI messages is determined by the application, by some means not specified in this Recommendation | 
        //    International Standard.
        //

        int8_t NalHrdBpPresentFlag = sps->vui_parameters_present_flag && sps->vui_seq_parameters->nal_hrd_parameters_present_flag ? 1 : 0;
        int8_t VclHrdBpPresentFlag = sps->vui_parameters_present_flag && sps->vui_seq_parameters->vcl_hrd_parameters_present_flag ? 1 : 0;
        
        // Hint :  initial_cpb_removal_delay and initial_cpb_removal_delay_offset
        //         The syntax element has a length in bits given by initial_cpb_removal_delay_length_minus1 + 1.
        // See also : ISO 14496/10(2020) - D.2.2 Buffering period SEI message semantics

        if (NalHrdBpPresentFlag)
        {
            int32_t cpb_cnt_minus1 = sps->vui_seq_parameters->nal_hrd_parameters->cpb_cnt_minus1;
            bp->initial_cpb_removal_delay.resize(cpb_cnt_minus1 + 1);
            bp->initial_cpb_removal_delay_offset.resize(cpb_cnt_minus1 + 1);
            for (int32_t SchedSelIdx=0; SchedSelIdx<=cpb_cnt_minus1; SchedSelIdx++)
            {
                br->U(sps->vui_seq_parameters->nal_hrd_parameters->initial_cpb_removal_delay_length_minus1 + 1, bp->initial_cpb_removal_delay[SchedSelIdx]);
                br->U(sps->vui_seq_parameters->nal_hrd_parameters->initial_cpb_removal_delay_length_minus1 + 1, bp->initial_cpb_removal_delay_offset[SchedSelIdx]);
            }
        }

        if (VclHrdBpPresentFlag)
        {
            int32_t cpb_cnt_minus1 = sps->vui_seq_parameters->vcl_hrd_parameters->cpb_cnt_minus1;
            bp->initial_cpb_removal_delay.resize(cpb_cnt_minus1 + 1);
            bp->initial_cpb_removal_delay_offset.resize(cpb_cnt_minus1 + 1);
            for (int32_t SchedSelIdx=0; SchedSelIdx<=cpb_cnt_minus1; SchedSelIdx++)
            {
                br->U(sps->vui_seq_parameters->vcl_hrd_parameters->initial_cpb_removal_delay_length_minus1 + 1, bp->initial_cpb_removal_delay[SchedSelIdx]);
                br->U(sps->vui_seq_parameters->vcl_hrd_parameters->initial_cpb_removal_delay_length_minus1 + 1, bp->initial_cpb_removal_delay_offset[SchedSelIdx]);
            }
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeSeiUserDataRegisteredSyntax(H26xBinaryReader::ptr br, uint32_t payloadSize, H264SeiUserDataRegisteredSyntax::ptr udr)
{
    // See also : ISO 14496/10(2020) - D.1.6 User data registered by ITU-T Rec. T.35 SEI message syntax
    try
    {
        uint32_t i = 0;
        br->U(8, udr->itu_t_t35_country_code);
        if (udr->itu_t_t35_country_code != 0xFF)
        {
            i = 1;
        }
        else
        {
            br->U(8, udr->itu_t_t35_country_code_extension_byte);
            i = 2;
        }
        udr->itu_t_t35_payload_byte.resize(payloadSize - i);
        size_t index = 0;
        do
        {
            br->U(8, udr->itu_t_t35_payload_byte[index]);
            index++;
            i++;
        } while (i<payloadSize);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeSeiUserDataUnregisteredSyntax(H26xBinaryReader::ptr br, uint32_t payloadSize, H264SeiUserDataUnregisteredSyntax::ptr udn)
{
    // See also : ISO 14496/10(2020) - D.1.7 User data unregistered SEI message syntax
    try
    {
        for (size_t i=0; i<16; i++)
        {
            br->U(8, udn->uuid_iso_iec_11578[i]);
        }
        udn->user_data_payload_byte.resize(payloadSize - 16);
        for (uint16_t i=16; i<payloadSize; i++)
        {
            br->U(8, udn->user_data_payload_byte[i-16]);
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeSeiPictureTimingSyntax(H26xBinaryReader::ptr br, H264VuiSyntax::ptr vui, H264SeiPictureTimingSyntax::ptr pt)
{
    // See also : ISO 14496/10(2020) - D.1.2 Buffering period SEI message syntax
    try
    {
        // The variable CpbDpbDelaysPresentFlag is derived as follows:
        //     – If any of the following is true, the value of CpbDpbDelaysPresentFlag shall be set equal to 1:
        //     – nal_hrd_parameters_present_flag is present in the bitstream and is equal to 1,
        //     – vcl_hrd_parameters_present_flag is present in the bitstream and is equal to 1,
        //     – the need for the presence of CPB and DPB output delays in the bitstream in picture timing SEI messages is 
        //     determined by the application, by some means not specified in this Recommendation | International Standard.
        //     – Otherwise, the value of CpbDpbDelaysPresentFlag shall be set equal to 0.
        //
        int8_t CpbDpbDelaysPresentFlag = vui->nal_hrd_parameters_present_flag || vui->vcl_hrd_parameters_present_flag ? 1 : 0;
        if (CpbDpbDelaysPresentFlag)
        {
            // Hint : cpb_removal_delay
            // NOTE 2 The value of cpb_removal_delay_length_minus1 that determines the length (in bits) of the syntax element 
            // cpb_removal_delay is the value of cpb_removal_delay_length_minus1 coded in the sequence parameter set that is active for the 
            // primary coded picture associated with the picture timing SEI message, although cpb_removal_delay specifies a number of clock ticks 
            // relative to the removal time of the preceding access unit containing a buffering period SEI message, which could be an access unit of a 
            // different coded video sequence.
            //
            // Hint : dpb_output_delay
            // The length of the syntax element dpb_output_delay is given in bits by dpb_output_delay_length_minus1 + 1. When 
            // max_dec_frame_buffering is equal to 0, dpb_output_delay shall be equal to 0.
            //
            if (vui->nal_hrd_parameters_present_flag)
            {
                br->U(vui->nal_hrd_parameters->cpb_removal_delay_length_minus1, pt->cpb_removal_delay);
                br->U(vui->nal_hrd_parameters->dpb_output_delay_length_minus1 + 1, pt->cpb_removal_delay);
            }
            else if (vui->vcl_hrd_parameters_present_flag)
            {
                br->U(vui->vcl_hrd_parameters->cpb_removal_delay_length_minus1, pt->cpb_removal_delay);
                br->U(vui->vcl_hrd_parameters->dpb_output_delay_length_minus1 + 1, pt->cpb_removal_delay);  
            }
        }

        if (vui->pic_struct_present_flag)
        {
            // See also : ISO 14496/10(2020) - Table D-1 – Interpretation of pic_struct
            // Hint : NumClockTS is determined by pic_struct as specified in Table D-1.
            int8_t NumClockTS[9] = {1, 1, 1, 2, 2, 3, 3, 2, 3};
            br->U(4, pt->pic_struct);
            if (pt->pic_struct > 9)
            {
                assert(false);
                return false;
            }
            pt->clock_timestamp_flag.resize(NumClockTS[pt->pic_struct]);
            pt->ct_type.resize(NumClockTS[pt->pic_struct]);
            pt->nuit_field_based_flag.resize(NumClockTS[pt->pic_struct]);
            pt->counting_type.resize(NumClockTS[pt->pic_struct]);
            pt->full_timestamp_flag.resize(NumClockTS[pt->pic_struct]);
            pt->discontinuity_flag.resize(NumClockTS[pt->pic_struct]);
            pt->cnt_dropped_flag.resize(NumClockTS[pt->pic_struct]);
            pt->n_frames.resize(NumClockTS[pt->pic_struct]);
            pt->seconds_value.resize(NumClockTS[pt->pic_struct]);
            pt->minutes_value.resize(NumClockTS[pt->pic_struct]);
            pt->hours_value.resize(NumClockTS[pt->pic_struct]);
            pt->seconds_flag.resize(NumClockTS[pt->pic_struct]);
            pt->minutes_flag.resize(NumClockTS[pt->pic_struct]);
            pt->hours_flag.resize(NumClockTS[pt->pic_struct]);
            pt->time_offset.resize(NumClockTS[pt->pic_struct]);
            for (int8_t i=0; i<NumClockTS[pt->pic_struct]; i++)
            {
                br->U(1, pt->clock_timestamp_flag[i]);
                if (pt->clock_timestamp_flag[i])
                {
                    br->U(2, pt->ct_type[i]);
                    br->U(1, pt->nuit_field_based_flag[i]);
                    br->U(5, pt->counting_type[i]);
                    br->U(1, pt->full_timestamp_flag[i]);
                    br->U(1, pt->discontinuity_flag[i]);
                    br->U(1, pt->cnt_dropped_flag[i]);
                    br->U(8, pt->n_frames[i]);
                    if (pt->full_timestamp_flag[i])
                    {
                        br->U(6, pt->seconds_value[i]);
                        br->U(6, pt->minutes_value[i]);
                        br->U(5, pt->hours_value[i]);
                    }
                    else
                    {
                        br->U(1, pt->seconds_flag[i]);
                        if (pt->seconds_flag[i])
                        {
                            br->U(6, pt->seconds_value[i]);
                            br->U(1, pt->minutes_flag[i]);
                            if (pt->minutes_flag[i])
                            {
                                br->U(6, pt->minutes_value[i]);
                                br->U(1, pt->hours_flag[i]);
                                if (pt->hours_flag[i])
                                {
                                    br->U(5, pt->hours_value[i]);
                                }
                            }
                        }
                    }
                }
                // Hint :
                // time_offset_length greater than 0 specifies the length in bits of the time_offset syntax element. time_offset_length equal 
                // to 0 specifies that the time_offset syntax element is not present. When the time_offset_length syntax element is present in 
                // more than one hrd_parameters( ) syntax structure within the VUI parameters syntax structure, the value of the 
                // time_offset_length parameters shall be equal in both hrd_parameters( ) syntax structures. When the time_offset_length 
                // syntax element is not present, it shall be inferred to be equal to 24.
                int32_t time_offset_length = 0;
                if (vui->nal_hrd_parameters_present_flag)
                {
                    time_offset_length = vui->nal_hrd_parameters->time_offset_length;
                }
                else if (vui->vcl_hrd_parameters_present_flag)
                {
                    time_offset_length = vui->vcl_hrd_parameters->time_offset_length;
                }
                if (time_offset_length > 0)
                {
                    br->I(time_offset_length, pt->time_offset[i]);
                }
            }
        }

        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeSeiRecoveryPointSyntax(H26xBinaryReader::ptr br, H264SeiRecoveryPointSyntax::ptr pt)
{
    // See also : ISO 14496/10(2020) - D.1.8 Recovery point SEI message syntax
    try
    {
        br->UE(pt->recovery_frame_cnt);
        br->U(1, pt->exact_match_flag);
        br->U(1, pt->broken_link_flag);
        br->U(2, pt->changing_slice_group_idc);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeSeiContentLigntLevelInfoSyntax(H26xBinaryReader::ptr br, H264SeiContentLigntLevelInfoSyntax::ptr clli)
{
    // See also : ISO 14496/10(2020) - D.1.31 Content light level information SEI message syntax
    try
    {
        br->U(16, clli->max_content_light_level);
        br->U(16, clli->max_pic_average_light_level);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeSeiDisplayOrientationSyntax(H26xBinaryReader::ptr br, H264SeiDisplayOrientationSyntax::ptr dot)
{
    // See also : ISO 14496/10(2020) - D.1.27 Display orientation SEI message syntax
    try 
    {
        br->U(1, dot->display_orientation_cancel_flag);
        if (dot->display_orientation_cancel_flag)
        {
            br->U(1, dot->hor_flip);
            br->U(1, dot->ver_flip);
            br->U(16, dot->anticlockwise_rotation);
            br->UE(dot->display_orientation_repetition_period);
            br->U(1, dot->display_orientation_extension_flag);
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeSeiMasteringDisplayColourVolumeSyntax(H26xBinaryReader::ptr br, H264MasteringDisplayColourVolumeSyntax::ptr mdcv)
{
    // See also : ISO 14496/10(2020) - D.1.29 Mastering display colour volume SEI message syntax
    try 
    {
        for (size_t c=0; c<3; c++)
        {
            br->U(16, mdcv->display_primaries_x[c]);
            br->U(16, mdcv->display_primaries_y[c]);
        }
        br->U(16, mdcv->white_point_x);
        br->U(16, mdcv->white_point_y);
        br->U(32, mdcv->max_display_mastering_luminance);
        br->U(32, mdcv->min_display_mastering_luminance);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeSeiFilmGrainSyntax(H26xBinaryReader::ptr br, H264SeiFilmGrainSyntax::ptr fg)
{
    // See also : ISO 14496/10(2020) - D.1.21 Film grain characteristics SEI message syntax
    try 
    {
        br->U(1, fg->film_grain_characteristics_cancel_flag);
        if (!fg->film_grain_characteristics_cancel_flag)
        {
            br->U(2, fg->film_grain_model_id);
            br->U(1, fg->separate_colour_description_present_flag);
            if (!fg->separate_colour_description_present_flag)
            {
                br->U(3, fg->film_grain_bit_depth_luma_minus8);
                br->U(3, fg->film_grain_bit_depth_chroma_minus8);
                br->U(1, fg->film_grain_full_range_flag);
                br->U(8, fg->film_grain_colour_primaries);
                br->U(8, fg->film_grain_transfer_characteristics);
                br->U(8, fg->film_grain_matrix_coefficients);
            }
            br->U(2, fg->blending_mode_id);
            br->U(4, fg->log2_scale_factor);
            fg->intensity_interval_lower_bound.resize(3);
            fg->intensity_interval_upper_bound.resize(3);
            fg->comp_model_value.resize(3);
            for (size_t c=0; c<3; c++)
            {
                br->U(1, fg->comp_model_present_flag[c]);
            }
            for (size_t c=0; c<3; c++)
            {
                if (fg->comp_model_present_flag[c])
                {
                    br->U(8, fg->num_intensity_intervals_minus1[c]);
                    br->U(3, fg->num_model_values_minus1[c]);
                    fg->intensity_interval_lower_bound[c].resize(fg->num_intensity_intervals_minus1[c] + 1);
                    fg->intensity_interval_upper_bound[c].resize(fg->num_intensity_intervals_minus1[c] + 1);
                    fg->comp_model_value[c].resize(fg->num_intensity_intervals_minus1[c] + 1);
                    for (uint32_t i=0; i<=fg->num_intensity_intervals_minus1[c]; i++)
                    {
                        br->U(8, fg->intensity_interval_lower_bound[c][i]);
                        br->U(8, fg->intensity_interval_upper_bound[c][i]);
                        fg->comp_model_value[c][i].resize(fg->num_model_values_minus1[c] + 1);
                        for (uint32_t j=0; j<=fg->num_model_values_minus1[c]; j++)
                        {
                            br->SE(fg->comp_model_value[c][i][j]);
                        }
                    }
                }
            }
            br->UE(fg->film_grain_characteristics_repetition_period);
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeSeiFramePackingArrangementSyntax(H26xBinaryReader::ptr br, H264SeiFramePackingArrangementSyntax::ptr fpa)
{
    // See also : ISO 14496/10(2020) - D.1.27 Display orientation SEI message syntax
    try
    {
        br->UE(fpa->frame_packing_arrangement_id);
        br->U(1, fpa->frame_packing_arrangement_cancel_flag);
        if (!fpa->frame_packing_arrangement_cancel_flag)
        {
            br->U(7, fpa->frame_packing_arrangement_type);
            br->U(1, fpa->quincunx_sampling_flag);
            br->U(6, fpa->content_interpretation_type);
            br->U(1, fpa->spatial_flipping_flag);
            br->U(1, fpa->frame0_flipped_flag);
            br->U(1, fpa->field_views_flag);
            br->U(1, fpa->current_frame_is_frame0_flag);
            br->U(1, fpa->frame0_self_contained_flag);
            br->U(1, fpa->frame1_self_contained_flag);
            if (!fpa->quincunx_sampling_flag && fpa->frame_packing_arrangement_type != 5)
            {
                br->U(4, fpa->frame0_grid_position_x);
                br->U(4, fpa->frame0_grid_position_y);
                br->U(4, fpa->frame1_grid_position_x);
                br->U(4, fpa->frame1_grid_position_y);
            }
            br->U(8, fpa->frame_packing_arrangement_reserved_byte);
            br->UE(fpa->frame_packing_arrangement_repetition_period);
        }
        br->U(1, fpa->frame_packing_arrangement_extension_flag);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeSeiAlternativeTransferCharacteristicsSyntax(H26xBinaryReader::ptr br, H264SeiAlternativeTransferCharacteristicsSyntax::ptr atc)
{
    // See also : D.1.32 Alternative transfer characteristics SEI message syntax
    try
    {
        br->U(8, atc->preferred_transfer_characteristics);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool H264Deserialize::DeserializeAmbientViewingEnvironmentSyntax(H26xBinaryReader::ptr br, H264AmbientViewingEnvironmentSyntax::ptr awe)
{
    // See also : ISO 14496/10(2020) - D.1.34 Ambient viewing environment SEI message syntax
    try
    {
        br->U(32, awe->ambient_illuminance);
        br->U(16, awe->ambient_light_x);
        br->U(16, awe->ambient_light_y);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

} // namespace Codec
} // namespace Mmp