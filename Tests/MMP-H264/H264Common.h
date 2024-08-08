//
// H264Common.h
//
// Library: Codec
// Package: H264
// Module:  H264
// 

#pragma once

#include <set>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <unordered_map>

namespace Mmp
{
namespace Codec
{

/**
 * @sa 1 - ISO 14496/10(2020) - A.2 Profiles
 *     2 - ISO 14496/10(2020) - H.6.1 Profiles
 */
enum H264Profile 
{
    MMP_H264_PROFILE_FREXT_CAVLC444     = 44,   // A.2.11 CAVLC 4:4:4 Intra profile
    MMP_H264_PROFILE_BASELINE           = 66,   // A.2.1 Baseline profile
    MMP_H264_PROFILE_MAIN               = 77,   // A.2.2 Main profile
    MMP_H264_PROFILE_EXTENDED           = 88,   // A.2.3 Extended profile
    MMP_H264_PROFILE_HIGH               = 100,  // A.2.4 High profile
    MMP_H264_PROFILE_HIGH10             = 110,  // A.2.5 High 10 profile
    MMP_H264_PROFILE_HIGH422            = 122,  // A.2.6 High 4:2:2 profile and A.2.9 High 4:2:2 Intra profile
    MMP_H264_PROFILE_HIGH444            = 244,  // A.2.7 High 4:4:4 Predictive profile and A.2.10 High 4:4:4 Intra profile
    MMP_H264_PROFILE_MVC_HIGH           = 118,  // H.6.1.2 MFC Depth High profile
    MMP_H264_PROFILE_STEREO_HIGH        = 128   
};

/**
 * @sa ISO 14496/10(2020) - Table 6-1 – SubWidthC, and SubHeightC values derived from chroma_format_idc and separate_colour_plane_flag 
 */
enum H264ChromaFormat
{
    MMP_H264_CHROMA_400                 = 0,    //!< Monochrome
    MMP_H264_CHROMA_420                 = 1,    //!< 4:2:0
    MMP_H264_CHROMA_422                 = 2,    //!< 4:2:2
    MMP_H264_CHROMA_444                 = 3     //!< 4:4:4
};

/**
 * @sa ISO 14496/10(2020) - Table 7-6 – Name association to slice_type 
 */
enum H264SliceType
{
    MMP_H264_P_SLICE                    = 0,
    MMP_H264_B_SLICE                    = 1,
    MMP_H264_I_SLICE                    = 2,
    MMP_H264_SP_SLICE                   = 3,
    MMP_H264_SI_SLICE                   = 4,
    MMP_H264_NUM_SLICE_TYPES            = 5
};

/**
 * @sa  ISO 14496/10(2020) - Table 7-1 – NAL unit type codes, syntax element categories, and NAL unit type classes
 */
enum H264NaluType
{
    MMP_H264_NALU_TYPE_NULL             = 0,
    MMP_H264_NALU_TYPE_SLICE            = 1,
    MMP_H264_NALU_TYPE_DPA              = 2,
    MMP_H264_NALU_TYPE_DPB              = 3,
    MMP_H264_NALU_TYPE_DPC              = 4,
    MMP_H264_NALU_TYPE_IDR              = 5,
    MMP_H264_NALU_TYPE_SEI              = 6,
    MMP_H264_NALU_TYPE_SPS              = 7,
    MMP_H264_NALU_TYPE_PPS              = 8,
    MMP_H264_NALU_TYPE_AUD              = 9,
    MMP_H264_NALU_TYPE_EOSEQ            = 10,
    MMP_H264_NALU_TYPE_EOSTREAM         = 11,
    MMP_H264_NALU_TYPE_FILL             = 12,
    MMP_H264_NALU_TYPE_SPSEXT           = 13,
    MMP_H264_NALU_TYPE_PREFIX           = 14,
    MMP_H264_NALU_TYPE_SUB_SPS          = 15,
    MMP_H264_NALU_TYPE_SLICE_AUX        = 19,
    MMP_H264_NALU_TYPE_SLC_EXT          = 20,
    MMP_H264_NALU_TYPE_VDRD             = 24
};

/**
 * @sa  ISO 14496/10(2020) - Table 7-9 – Memory management control operation (memory_management_control_operation) values
 */
enum H264MmcoType
{
    MMP_H264_MMOO_0 = 0,    // End memory_management_control_operation 
                            // syntax element loop
    MMP_H264_MMOO_1 = 1,    // Mark a short-term reference picture as
                            // "unused for reference"
    MMP_H264_MMOO_2 = 2,    // Mark a long-term reference picture as
                            // "unused for reference"
    MMP_H264_MMOO_3 = 3,    // Mark a short-term reference picture as
                            // "used for long-term reference" and assign a
                            // long-term frame index to it
    MMP_H264_MMOO_4 = 4,    // Specify the maximum long-term frame index
                            // and mark all long-term reference pictures
                            // having long-term frame indices greater than
                            // the maximum value as "unused for reference"
    MMP_H264_MMOO_5 = 5,    // Mark all reference pictures as
                            // "unused for reference" and set the
                            // MaxLongTermFrameIdx variable to
                            // "no long-term frame indices"
    MMP_H264_MMOO_6 = 6     // Mark the current picture as
                            // "used for long-term reference" and assign a
                            // long-term frame index to it
};

/**
 * @sa ISO 14496/10(2020) - D.1.1 General SEI message syntax
 */
enum H264SeiType
{
    MMP_H264_SEI_BUFFERING_PERIOD                            = 0,
    MMP_H264_SEI_PIC_TIMING                                  = 1,
    MMP_H264_SEI_PAN_SCAN_RECT                               = 2,
    MMP_H264_SEI_FILLER_PAYLOAD                              = 3,
    MMP_H264_SEI_USER_DATA_REGISTERED_ITU_T_T35              = 4,
    MMP_H264_SEI_USER_DATA_UNREGISTERED                      = 5,
    MMP_H264_SEI_RECOVERY_POINT                              = 6,
    MMP_H264_SEI_DEC_REF_PIC_MARKING_REPETITION              = 7,
    MMP_H264_SEI_SPARE_PIC                                   = 8,
    MMP_H264_SEI_SCENE_INFO                                  = 9,
    MMP_H264_SEI_SUB_SEQ_INFO                                = 10,
    MMP_H264_SEI_SUB_SEQ_LAYER_CHARACTERISTICS               = 11,
    MMP_H264_SEI_SUB_SEQ_CHARACTERISTICS                     = 12,
    MMP_H264_SEI_FULL_FRAME_FREEZE                           = 13,
    MMP_H264_SEI_FULL_FRAME_FREEZE_RELEASE                   = 14,
    MMP_H264_SEI_FULL_FRAME_SNAPSHOT                         = 15,
    MMP_H264_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START        = 16,
    MMP_H264_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END          = 17,
    MMP_H264_SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET          = 18,
    MMP_H264_SEI_FILM_GRAIN_CHARACTERISTICS                  = 19,
    MMP_H264_SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE        = 20,
    MMP_H264_SEI_STEREO_VIDEO_INFO                           = 21,
    MMP_H264_SEI_POST_FILTER_HINTS                           = 22,
    MMP_H264_SEI_TONE_MAPPING                                = 23,
    MMP_H264_SEI_SCALABILITY_INFO                            = 24,
    MMP_H264_SEI_SUB_PIC_SCALABLE_LAYER                      = 25,
    MMP_H264_SEI_NON_REQUIRED_LAYER_REP                      = 26,
    MMP_H264_SEI_PRIORITY_LAYER_INFO                         = 27,
    MMP_H264_SEI_LAYERS_NOT_PRESENT                          = 28,
    MMP_H264_SEI_LAYER_DEPENDENCY_CHANGE                     = 29,
    MMP_H264_SEI_SCALABLE_NESTING                            = 30,
    MMP_H264_SEI_BASE_LAYER_TEMPORAL_HRD                     = 31,
    MMP_H264_SEI_QUALITY_LAYER_INTEGRITY_CHECK               = 32,
    MMP_H264_SEI_REDUNDANT_PIC_PROPERTY                      = 33,
    MMP_H264_SEI_TL0_DEP_REP_INDEX                           = 34,
    MMP_H264_SEI_TL_SWITCHING_POINT                          = 35,
    MMP_H264_SEI_PARALLEL_DECODING_INFO                      = 36,
    MMP_H264_SEI_MVC_SCALABLE_NESTING                        = 37,
    MMP_H264_SEI_VIEW_SCALABILITY_INFO                       = 38,
    MMP_H264_SEI_MULTIVIEW_SCENE_INFO                        = 39,
    MMP_H264_SEI_MULTIVIEW_ACQUISITION_INFO                  = 40,
    MMP_H264_SEI_NON_REQUIRED_VIEW_COMPONENT                 = 41,
    MMP_H264_SEI_VIEW_DEPENDENCY_CHANGE                      = 42,
    MMP_H264_SEI_OPERATION_POINTS_NOT_PRESENT                = 43,
    MMP_H264_SEI_BASE_VIEW_TEMPORAL_HRD                      = 44,
    MMP_H264_SEI_FRAME_PACKING_ARRANGEMENT                   = 45,
    MMP_H264_SEI_MULTIVIEW_VIEW_POSITION_4                   = 46,
    MMP_H264_SEI_DISPLAY_ORIENTATION                         = 47,
    MMP_H264_SEI_MVCD_SCALABLE_NESTING                       = 48,
    MMP_H264_SEI_MVCD_VIEW_SCALABILITY_INFO                  = 49,
    MMP_H264_SEI_DEPTH_REPRESENTATION_INFO_4                 = 50,
    MMP_H264_SEI_THREE_DIMENSIONAL_REFERENCE_DISPLAYS_INFO_4 = 51,
    MMP_H264_SEI_DEPTH_TIMING                                = 52,
    MMP_H264_SEI_DEPTH_SAMPLING_INFO                         = 53,
    MMP_H264_SEI_CONSTRAINED_DEPTH_PARAMETER_SET_IDENTIFIER  = 54,
    MMP_H264_SEI_GREEN_METADATA                              = 56,
    MMP_H264_SEI_MASTERING_DISPLAY_COLOUR_VOLUME             = 137,
    MMP_H264_SEI_CONTENT_LIGHT_LEVEL_INFO                    = 144,
    MMP_H264_SEI_ALTERNATIVE_TRANSFER_CHARACTERISTICS        = 147,
    MP_H264_SEI_AMBIENT_VIEWING_ENVIRONMENT                  = 148
};

/**
 * @sa ISO 14496/10(2020) - E.1.2 HRD parameters syntax
 */
class H264HrdSyntax
{
public:
    using ptr = std::shared_ptr<H264HrdSyntax>;
public:
    H264HrdSyntax();
    ~H264HrdSyntax() = default;
public:
    uint32_t               cpb_cnt_minus1;
    uint32_t               bit_rate_scale;
    uint32_t               cpb_size_scale;
    std::vector<uint32_t>  bit_rate_value_minus1;
    std::vector<uint32_t>  cpb_size_value_minus1;
    std::vector<uint32_t>  cbr_flag;
    uint32_t               initial_cpb_removal_delay_length_minus1;
    uint32_t               cpb_removal_delay_length_minus1;
    uint32_t               dpb_output_delay_length_minus1;
    uint32_t               time_offset_length;
};

/**
 * @sa ISO 14496/10(2020) - E.1.1 VUI parameters syntax
 */
class H264VuiSyntax
{
public:
    using ptr = std::shared_ptr<H264VuiSyntax>;
public:
    H264VuiSyntax();
    ~H264VuiSyntax() = default;
public:
    uint8_t   aspect_ratio_info_present_flag;
    uint8_t   aspect_ratio_idc;
    uint16_t  sar_width;
    uint16_t  sar_height;
    uint8_t   overscan_info_present_flag;
    uint8_t   overscan_appropriate_flag;
    uint8_t   video_signal_type_present_flag;
    uint8_t   video_format;
    uint8_t   video_full_range_flag;
    uint8_t   colour_description_present_flag;
    uint32_t  colour_primaries;
    uint32_t  transfer_characteristics;
    uint32_t  matrix_coefficients;
    uint8_t   chroma_location_info_present_flag;
    uint32_t  chroma_sample_loc_type_top_field;
    uint32_t  chroma_sample_loc_type_bottom_field;
    uint8_t   timing_info_present_flag;
    uint32_t  num_units_in_tick;
    uint32_t  time_scale;
    uint8_t   fixed_frame_rate_flag;
    uint8_t   nal_hrd_parameters_present_flag;
    H264HrdSyntax::ptr nal_hrd_parameters;
    uint8_t   vcl_hrd_parameters_present_flag;
    H264HrdSyntax::ptr vcl_hrd_parameters;
    uint8_t   low_delay_hrd_flag;
    uint8_t   pic_struct_present_flag;
    uint8_t   bitstream_restriction_flag;
    uint8_t   motion_vectors_over_pic_boundaries_flag;
    uint32_t  max_bytes_per_pic_denom;
    uint32_t  max_bits_per_mb_denom;
    uint32_t  log2_max_mv_length_vertical;
    uint32_t  log2_max_mv_length_horizontal;
    uint32_t  num_reorder_frames;
    uint32_t  max_dec_frame_buffering;
};

/**
 * @sa ISO 14496/10(2020) - F.3.3.1.1 NAL unit header SVC extension syntax 
 */
class H264NalSvcSyntax
{
public:
    using ptr = std::shared_ptr<H264NalSvcSyntax>;
public:
    H264NalSvcSyntax();
    ~H264NalSvcSyntax() = default;
public:
    uint8_t   idr_flag;
    uint8_t   priority_id;
    uint8_t   no_inter_layer_pred_flag;
    uint8_t   dependency_id;
    uint8_t   quality_id;
    uint8_t   temporal_id;
    uint8_t   use_ref_base_pic_flag;
    uint8_t   discardable_flag;
    uint8_t   output_flag;
    uint8_t   reserved_three_2bits;
};

/**
 * @sa ISO 14496/10(2020) - I.3.3.1.1 NAL unit header 3D-AVC extension syntax
 */
class H264Nal3dAvcSyntax
{
public:
    using ptr = std::shared_ptr<H264Nal3dAvcSyntax>;
public:
    H264Nal3dAvcSyntax();
    ~H264Nal3dAvcSyntax() = default;
public:
    uint8_t    view_idx;
    uint8_t    depth_flag;
    uint8_t    non_idr_flag;
    uint8_t    temporal_id;
    uint8_t    anchor_pic_flag;
    uint8_t    inter_view_flag;
};

/**
 * @sa  ISO 14496/10(2020) - G.3.3.1.1 NAL unit header MVC extension syntax
 */
class H264NalMvcSyntax
{
public:
    using ptr = std::shared_ptr<H264NalMvcSyntax>;
public:
    H264NalMvcSyntax();
    ~H264NalMvcSyntax() = default;
public:
    uint8_t   non_idr_flag;
    uint8_t   priority_id;
    uint16_t  view_id;
    uint8_t   temporal_id;
    uint8_t   anchor_pic_flag;
    uint8_t   inter_view_flag;
    uint8_t   reserved_one_bit;
};


/**
 * @sa ISO 14496/10(2020) - D.1.2 Buffering period SEI message syntax
 */
class H264SeiBufferPeriodSyntax
{
public:
    using ptr = std::shared_ptr<H264SeiBufferPeriodSyntax>;
public:
    H264SeiBufferPeriodSyntax();
    ~H264SeiBufferPeriodSyntax() = default;
public:
    uint32_t seq_parameter_set_id;
    std::vector<uint32_t> initial_cpb_removal_delay;
    std::vector<uint32_t> initial_cpb_removal_delay_offset;    
};

/**
 * @sa ISO 14496/10(2020) - D.1.3 Picture timing SEI message syntax
 */
class H264SeiPictureTimingSyntax
{
public:
    using ptr = std::shared_ptr<H264SeiPictureTimingSyntax>;
public:
    H264SeiPictureTimingSyntax();
    ~H264SeiPictureTimingSyntax() = default;
public:
    uint32_t  cpb_removal_delay;
    uint32_t  dpb_output_delay;
    uint8_t   pic_struct;
    std::vector<uint8_t>   clock_timestamp_flag;
    std::vector<uint8_t>   ct_type;
    std::vector<uint8_t>   nuit_field_based_flag;
    std::vector<uint8_t>   counting_type;
    std::vector<uint8_t>   full_timestamp_flag;
    std::vector<uint8_t>   discontinuity_flag;
    std::vector<uint8_t>   cnt_dropped_flag;
    std::vector<uint8_t>  n_frames;
    std::vector<uint8_t>   seconds_value;
    std::vector<uint8_t>   minutes_value;
    std::vector<uint8_t>   hours_value;
    std::vector<uint8_t>   seconds_flag;
    std::vector<uint8_t>   minutes_flag;
    std::vector<uint8_t>   hours_flag;
    std::vector<int32_t>  time_offset;
};

/**
 * @sa ISO 14496/10(2020) - D.1.6 User data registered by ITU-T Rec. T.35 SEI message syntax
 */
class H264SeiUserDataRegisteredSyntax
{
public:
    using ptr = std::shared_ptr<H264SeiUserDataRegisteredSyntax>;
public:
    H264SeiUserDataRegisteredSyntax();
    ~H264SeiUserDataRegisteredSyntax() = default;
public:
    uint8_t  itu_t_t35_country_code;
    uint8_t  itu_t_t35_country_code_extension_byte;
    std::vector<uint8_t> itu_t_t35_payload_byte;
};

/**
 * @sa ISO 14496/10(2020) - D.1.7 User data unregistered SEI message syntax
 */
class H264SeiUserDataUnregisteredSyntax
{
public:
    using ptr = std::shared_ptr<H264SeiUserDataUnregisteredSyntax>;
public:
    H264SeiUserDataUnregisteredSyntax();
    ~H264SeiUserDataUnregisteredSyntax() = default;
public:
    uint8_t uuid_iso_iec_11578[16];
    std::vector<uint8_t> user_data_payload_byte;
};

/**
 * @sa ISO 14496/10(2020) - D.1.8 Recovery point SEI message syntax
 */
class H264SeiRecoveryPointSyntax
{
public:
    using ptr = std::shared_ptr<H264SeiRecoveryPointSyntax>;
public:
    H264SeiRecoveryPointSyntax();
    ~H264SeiRecoveryPointSyntax() = default;
public:
    uint32_t recovery_frame_cnt;
    uint8_t  exact_match_flag;
    uint8_t  broken_link_flag;
    uint8_t  changing_slice_group_idc;
};

/**
 * @sa  ISO 14496/10(2020) - D.1.21 Film grain characteristics SEI message syntax
 */
class H264SeiFilmGrainSyntax
{
public:
    using ptr = std::shared_ptr<H264SeiFilmGrainSyntax>;
public:
    H264SeiFilmGrainSyntax();
    ~H264SeiFilmGrainSyntax() = default;
public:
    uint8_t  film_grain_characteristics_cancel_flag;
    uint8_t  film_grain_model_id;
    uint8_t  separate_colour_description_present_flag;
    uint8_t  film_grain_bit_depth_luma_minus8;
    uint8_t  film_grain_bit_depth_chroma_minus8;
    uint8_t  film_grain_full_range_flag;
    uint8_t  film_grain_colour_primaries;
    uint8_t  film_grain_transfer_characteristics;
    uint8_t  film_grain_matrix_coefficients;
    uint8_t  blending_mode_id;
    uint8_t  log2_scale_factor;
    uint8_t  comp_model_present_flag[3];
    uint8_t  num_intensity_intervals_minus1[3];
    uint8_t  num_model_values_minus1[3];
    std::vector<std::vector<uint8_t>>  intensity_interval_lower_bound;
    std::vector<std::vector<uint8_t>>  intensity_interval_upper_bound;
    std::vector<std::vector<std::vector<int32_t>>>  comp_model_value;
    uint32_t film_grain_characteristics_repetition_period;
};

/**
 * @sa  ISO 14496/10(2020) - D.1.26 Frame packing arrangement SEI message syntax
 */
class H264SeiFramePackingArrangementSyntax
{
public:
    using ptr = std::shared_ptr<H264SeiFramePackingArrangementSyntax>;
public:
    H264SeiFramePackingArrangementSyntax();
    ~H264SeiFramePackingArrangementSyntax() = default;
public:
    uint32_t  frame_packing_arrangement_id;
    uint8_t   frame_packing_arrangement_cancel_flag;
    uint8_t   frame_packing_arrangement_type;
    uint8_t   quincunx_sampling_flag;
    uint8_t   content_interpretation_type;
    uint8_t   spatial_flipping_flag;
    uint8_t   frame0_flipped_flag;
    uint8_t   field_views_flag;
    uint8_t   current_frame_is_frame0_flag;
    uint8_t   frame0_self_contained_flag;
    uint8_t   frame1_self_contained_flag;
    uint8_t   frame0_grid_position_x;
    uint8_t   frame0_grid_position_y;
    uint8_t   frame1_grid_position_x;
    uint8_t   frame1_grid_position_y;
    uint8_t   frame_packing_arrangement_reserved_byte;
    uint32_t  frame_packing_arrangement_repetition_period;
    uint8_t   frame_packing_arrangement_extension_flag;
};

/**
 * @sa ISO 14496/10(2020) - D.1.27 Display orientation SEI message syntax
 */
class H264SeiDisplayOrientationSyntax
{
public:
    using ptr = std::shared_ptr<H264SeiDisplayOrientationSyntax>;
public:
    H264SeiDisplayOrientationSyntax();
    ~H264SeiDisplayOrientationSyntax() = default;
public:
    uint8_t   display_orientation_cancel_flag;
    uint8_t   hor_flip;
    uint8_t   ver_flip;
    uint16_t  anticlockwise_rotation;
    uint32_t  display_orientation_repetition_period;
    uint8_t   display_orientation_extension_flag;
};

/**
 * @sa ISO 14496/10(2020) - D.1.29 Mastering display colour volume SEI message syntax
 */
class H264MasteringDisplayColourVolumeSyntax
{
public:
    using ptr = std::shared_ptr<H264MasteringDisplayColourVolumeSyntax>;
public:
    H264MasteringDisplayColourVolumeSyntax();
    ~H264MasteringDisplayColourVolumeSyntax() = default;
public:
    uint16_t  display_primaries_x[3];
    uint16_t  display_primaries_y[3];
    uint16_t  white_point_x;
    uint16_t  white_point_y;
    uint32_t  max_display_mastering_luminance;
    uint32_t  min_display_mastering_luminance;
};

/**
 * @sa  ISO 14496/10(2020) - D.1.31 Content light level information SEI message syntax
 */
class H264SeiContentLigntLevelInfoSyntax
{
public:
    using ptr = std::shared_ptr<H264SeiContentLigntLevelInfoSyntax>;
public:
    H264SeiContentLigntLevelInfoSyntax();
    ~H264SeiContentLigntLevelInfoSyntax() = default;
public:
    uint16_t max_content_light_level;
    uint16_t max_pic_average_light_level;
};

/**
 * @sa ISO 14496/10(2020) - D.1.32 Alternative transfer characteristics SEI message syntax
 */
class H264SeiAlternativeTransferCharacteristicsSyntax
{
public:
    using ptr = std::shared_ptr<H264SeiAlternativeTransferCharacteristicsSyntax>;
public:
    H264SeiAlternativeTransferCharacteristicsSyntax();
    ~H264SeiAlternativeTransferCharacteristicsSyntax() = default;
public:
    uint8_t preferred_transfer_characteristics;
};

/**
 * @sa  ISO 14496/10(2020) - D.1.34 Ambient viewing environment SEI message syntax
 */
class H264AmbientViewingEnvironmentSyntax
{
public:
    using ptr = std::shared_ptr<H264AmbientViewingEnvironmentSyntax>;
public:
    H264AmbientViewingEnvironmentSyntax();
    ~H264AmbientViewingEnvironmentSyntax() = default;
public:
    uint32_t  ambient_illuminance;
    uint16_t  ambient_light_x;
    uint16_t  ambient_light_y;
};

/**
 * @sa ISO 14496/10(2020) - 7.3.2.3.1 Supplemental enhancement information message syntax
 */
class H264SeiSyntax
{
public:
    using ptr = std::shared_ptr<H264SeiSyntax>;
public:
    H264SeiSyntax();
    ~H264SeiSyntax() = default;
public:
    uint64_t  payloadType;
    uint64_t  payloadSize;
public:
    H264SeiBufferPeriodSyntax::ptr                        bp;
    H264SeiPictureTimingSyntax::ptr                       pt;
    H264SeiRecoveryPointSyntax::ptr                       rp;
    H264SeiContentLigntLevelInfoSyntax::ptr               clli;
    H264SeiDisplayOrientationSyntax::ptr                  dot;
    H264SeiFilmGrainSyntax::ptr                           fg;
    H264SeiFramePackingArrangementSyntax::ptr             fpa;
    H264MasteringDisplayColourVolumeSyntax::ptr           mpvc;
    H264SeiUserDataRegisteredSyntax::ptr                  udr;
    H264SeiUserDataUnregisteredSyntax::ptr                udn;
    H264SeiAlternativeTransferCharacteristicsSyntax::ptr  atc;
    H264AmbientViewingEnvironmentSyntax::ptr              awe;
};

/**
 * @sa ISO 14496/10(2020) - F.3.3.2.1.4 Sequence parameter set SVC extension syntax 
 */
class H264SpsSvcSynctax
{
public:
    using ptr = std::shared_ptr<H264SpsSvcSynctax>;
public:
    H264SpsSvcSynctax();
    ~H264SpsSvcSynctax() = default;
public:
    uint8_t  inter_layer_deblocking_filter_control_present_flag;
    uint8_t  extended_spatial_scalability_idc;
    uint8_t  chroma_phase_x_plus1_flag;
    uint8_t  chroma_phase_y_plus1;
    uint8_t  seq_ref_layer_chroma_phase_x_plus1_flag;
    uint8_t  seq_ref_layer_chroma_phase_y_plus1;
    int32_t  seq_scaled_ref_layer_left_offse;
    int32_t  seq_scaled_ref_layer_top_offset;
    int32_t  seq_scaled_ref_layer_right_offse;
    int32_t  seq_scaled_ref_layer_bottom_offset;
    uint8_t  seq_tcoeff_level_prediction_flag;
    uint8_t  adaptive_tcoeff_level_prediction_flag;
    uint8_t  slice_header_restriction_flag;
};

/**
 * @sa ISO 14496/10(2020) - G.3.3.2.1.4 Sequence parameter set MVC extension syntax
 */
class H264SpsMvcSyntax
{
public:
    using ptr = std::shared_ptr<H264SpsMvcSyntax>;
public:
    H264SpsMvcSyntax();
    ~H264SpsMvcSyntax() = default;
public:
    uint32_t   num_views_minus1;
    std::vector<uint32_t> view_id;
    std::vector<uint32_t> num_anchor_refs_l0;
    std::vector<std::vector<uint32_t>>  anchor_ref_l0;
    std::vector<uint32_t> num_anchor_refs_l1;
    std::vector<std::vector<uint32_t>>  anchor_ref_l1;
    std::vector<uint32_t> num_non_anchor_refs_l0;
    std::vector<std::vector<uint32_t>> non_anchor_ref_l0;
    std::vector<uint32_t> num_non_anchor_refs_l1;
    std::vector<std::vector<uint32_t>> non_anchor_ref_l1;
    uint32_t num_level_values_signalled_minus1;
    std::vector<uint8_t> level_idc;
    std::vector<uint32_t> num_applicable_ops_minus1;
    std::vector<std::vector<uint8_t>> applicable_op_temporal_id;
    std::vector<std::vector<uint32_t>> applicable_op_num_target_views_minus1;
    std::vector<std::vector<std::vector<uint32_t>>>  applicable_op_target_view_id;
    std::vector<std::vector<uint32_t>> applicable_op_num_views_minus1;
};

/**
 * @sa  ISO 14496/10(2020) - G.10.1 MVC VUI parameters extension syntax
 */
class H264MvcVuiSyntax
{
public:
    using ptr = std::shared_ptr<H264MvcVuiSyntax>;
public:
    H264MvcVuiSyntax();
    ~H264MvcVuiSyntax() = default;
public:
    uint32_t vui_mvc_num_ops_minus1;
    std::vector<uint8_t> vui_mvc_temporal_id;
    std::vector<uint32_t> vui_mvc_num_target_output_views_minus1;
    std::vector<std::vector<uint32_t>> vui_mvc_view_id;
    std::vector<uint8_t> vui_mvc_timing_info_present_flag;
    std::vector<uint32_t> vui_mvc_num_units_in_tick;
    std::vector<uint32_t> vui_mvc_time_scale;
    std::vector<uint8_t> vui_mvc_fixed_frame_rate_flag;
    std::vector<uint8_t> vui_mvc_nal_hrd_parameters_present_flag;
    std::vector<H264HrdSyntax::ptr> nalHrds;
    std::vector<uint8_t> vui_mvc_vcl_hrd_parameters_present_flag;
    std::vector<H264HrdSyntax::ptr> vclHrds;
    std::vector<uint8_t> vui_mvc_low_delay_hrd_flag;
    std::vector<uint8_t> vui_mvc_pic_struct_present_flag;
};

/**
 * @sa  ISO 14496/10(2020) - 7.3.3.1 Reference picture list modification syntax
 */
class H264ReferencePictureListModificationSyntax
{
public:
    using ptr = std::shared_ptr<H264ReferencePictureListModificationSyntax>;
public:
    H264ReferencePictureListModificationSyntax();
    ~H264ReferencePictureListModificationSyntax() = default;
public:
    uint8_t   ref_pic_list_modification_flag_l0;
    uint8_t   ref_pic_list_modification_flag_l1;
    std::vector<uint32_t> modification_of_pic_nums_idcs;
    union modification_of_pic_nums_idcs_data
    {
        uint32_t  abs_diff_pic_num_minus1;
        uint32_t  long_term_pic_num;
    };
    std::vector<modification_of_pic_nums_idcs_data> modification_of_pic_nums_idcs_datas;
};

/**
 * @sa  ISO 14496/10(2020) - 7.3.3.2 Prediction weight table syntax
 */
class H264PredictionWeightTableSyntax
{
public:
    using ptr = std::shared_ptr<H264PredictionWeightTableSyntax>;
public:
    H264PredictionWeightTableSyntax();
    ~H264PredictionWeightTableSyntax() = default;
public:
    uint32_t  luma_log2_weight_denom;
    uint32_t  chroma_log2_weight_denom;
    std::vector<uint8_t> luma_weight_l0_flag;
    std::vector<int32_t> luma_weight_l0;
    std::vector<int32_t> luma_offset_l0;
    std::vector<uint8_t> chroma_weight_l0_flag;
    std::vector<std::vector<int32_t>>  chroma_weight_l0;
    std::vector<std::vector<int32_t>>  chroma_offset_l0;
    std::vector<uint8_t> luma_weight_l1_flag;
    std::vector<int32_t> luma_weight_l1;
    std::vector<int32_t> luma_offset_l1;
    std::vector<uint8_t> chroma_weight_l1_flag;
    std::vector<std::vector<int32_t>>  chroma_weight_l1;
    std::vector<std::vector<int32_t>>  chroma_offset_l1;
};

/**
 * @sa ISO 14496/10(2020) - 7.3.3.3 Decoded reference picture marking syntax
 */
class H264DecodedReferencePictureMarkingSyntax
{
public:
    using ptr = std::shared_ptr<H264DecodedReferencePictureMarkingSyntax>;
public:
    H264DecodedReferencePictureMarkingSyntax();
    ~H264DecodedReferencePictureMarkingSyntax() = default;
public:
    uint8_t   no_output_of_prior_pics_flag;
    uint8_t   long_term_reference_flag;
    uint8_t   adaptive_ref_pic_marking_mode_flag;
    std::vector<uint32_t> memory_management_control_operations;
    union memory_management_control_operations_data
    {
        uint32_t  difference_of_pic_nums_minus1;
        uint32_t  long_term_pic_num;
        uint32_t  long_term_frame_idx;
        uint32_t  max_long_term_frame_idx_plus1;
    };
    std::vector<memory_management_control_operations_data> memory_management_control_operations_datas;
};

/**
 * @sa ISO 14496/10(2020) - 7.3.2.1.3 Subset sequence parameter set RBSP syntax
 */
class H264SubSpsSyntax
{
public:
    using ptr = std::shared_ptr<H264SubSpsSyntax>;
public:
    H264SubSpsSyntax();
    ~H264SubSpsSyntax() = default;
public:
    uint8_t  svc_vui_parameters_present_flag;
    uint8_t  bit_equal_to_one;
    uint8_t  mvc_vui_parameters_present_flag;
    H264MvcVuiSyntax::ptr mvcVui;
    uint8_t  additional_extension2_flag;
    uint8_t  additional_extension2_data_flag;
    H264SpsMvcSyntax::ptr mvc;
};

/**
 * @sa ISO 14496/10(2020) - 7.4.2 Raw byte sequence payloads and RBSP trailing bits semantics 
 */
class H264SpsContext
{
public:
    using ptr = std::shared_ptr<H264SpsContext>;
public:
    H264SpsContext();
    ~H264SpsContext() = default;
public:
    uint32_t BitDepthY;                        // (7-3)
    uint32_t QpBdOffsetY;                      // (7-4)
    uint32_t BitDepthC;                        // (7-5)
    uint32_t QpBdOffsetC;                      // (7-6)
    uint32_t RawMbBits;                        // (7-7)
    uint32_t MaxFrameNum;                      // (7-10)
    uint32_t MaxPicOrderCntLsb;                // (7-11)
    int64_t  ExpectedDeltaPerPicOrderCntCycle; // (7-12)
    uint32_t PicWidthInMbs;                    // (7-13)
    uint32_t PicWidthInSamplesL;               // (7-14)
    uint32_t PicWidthInSamplesC;               // (7-15)
    uint32_t PicHeightInMapUnits;              // (7-16)
    uint32_t PicSizeInMapUnits;                // (7-17)
    uint32_t FrameHeightInMbs;                 // (7-18)
    int32_t  CropUnitX;                        // (7-19) , (7-21)
    int32_t  CropUnitY;                        // (7-20) , (7-22)
};

/**
 * @sa  ISO 14496/10(2020) - 7.3.2 Raw byte sequence payloads and RBSP trailing bits syntax
 */ 
class H264SpsSyntax
{
public:
    using ptr = std::shared_ptr<H264SpsSyntax>;
public:
    H264SpsSyntax();
    ~H264SpsSyntax() = default;
public:
    uint8_t    profile_idc;
    uint8_t    constraint_set0_flag;
    uint8_t    constraint_set1_flag;
    uint8_t    constraint_set2_flag;
    uint8_t    constraint_set3_flag;
    uint8_t    constraint_set4_flag;
    uint8_t    constraint_set5_flag;
    uint8_t    level_idc;
    uint32_t   seq_parameter_set_id;
    uint32_t   chroma_format_idc;
    uint8_t    separate_colour_plane_flag;
    uint32_t   bit_depth_luma_minus8;
    uint32_t   bit_depth_chroma_minus8;
    uint8_t    qpprime_y_zero_transform_bypass_flag;
    uint8_t    seq_scaling_matrix_present_flag;
    std::vector<uint8_t> seq_scaling_list_present_flag;
    std::vector<std::vector<int32_t>> ScalingList4x4;
    std::vector<int32_t> UseDefaultScalingMatrix4x4Flag;
    std::vector<std::vector<int32_t>> ScalingList8x8;
    std::vector<int32_t> UseDefaultScalingMatrix8x8Flag;
    uint32_t   log2_max_frame_num_minus4;
    uint32_t   pic_order_cnt_type;
    uint32_t   log2_max_pic_order_cnt_lsb_minus4;
    uint8_t    delta_pic_order_always_zero_flag;
    int32_t    offset_for_non_ref_pic;
    int32_t    offset_for_top_to_bottom_field;
    uint32_t   num_ref_frames_in_pic_order_cnt_cycle;
    std::vector<int32_t> offset_for_ref_frame;
    uint32_t   max_num_ref_frames;
    uint8_t    gaps_in_frame_num_value_allowed_flag;
    uint32_t   pic_width_in_mbs_minus1;
    uint32_t   pic_height_in_map_units_minus1;
    uint8_t    frame_mbs_only_flag;
    uint8_t    mb_adaptive_frame_field_flag;
    uint8_t    direct_8x8_inference_flag;
    uint8_t    frame_cropping_flag;
    uint32_t   frame_crop_left_offset;
    uint32_t   frame_crop_right_offset;
    uint32_t   frame_crop_top_offset;
    uint32_t   frame_crop_bottom_offset;
    uint8_t    vui_parameters_present_flag;
    H264VuiSyntax::ptr vui_seq_parameters;
    H264SpsContext::ptr context;
};

/**
 * @sa  ISO 14496/10(2020) - 7.3.2.2 Picture parameter set RBSP syntax
 */
class H264PpsSyntax
{
public:
    using ptr = std::shared_ptr<H264PpsSyntax>;
public:
    H264PpsSyntax();
    ~H264PpsSyntax() = default;
public:
    uint32_t  pic_parameter_set_id;
    uint32_t  seq_parameter_set_id;
    uint8_t   entropy_coding_mode_flag;
    uint8_t   bottom_field_pic_order_in_frame_present_flag;
    uint32_t  num_slice_groups_minus1;
    uint32_t  slice_group_map_type;
#if 0 // feature missing
    std::vector<uint32_t>  run_length_minus1;
    std::vector<uint32_t>  top_left;
    std::vector<uint32_t>  bottom_right;
    uint8_t   slice_group_change_direction_flag;
    uint32_t  slice_group_change_rate_minus1;
    uint32_t  pic_size_in_map_units_minus1;
    std::vector<uint32_t> slice_group_id;
#endif
    uint32_t num_ref_idx_l0_default_active_minus1;
    uint32_t num_ref_idx_l1_default_active_minus1;
    uint32_t weighted_pred_flag;
    uint32_t weighted_bipred_idc;
    int32_t  pic_init_qp_minus26;
    int32_t  pic_init_qs_minus26;
    int32_t  chroma_qp_index_offset;
    uint8_t  deblocking_filter_control_present_flag;
    uint8_t  constrained_intra_pred_flag;
    uint8_t  redundant_pic_cnt_present_flag;
    uint8_t  transform_8x8_mode_flag;
    uint8_t  pic_scaling_matrix_present_flag;
    std::vector<uint8_t> pic_scaling_list_present_flag;
    std::vector<std::vector<int32_t>> ScalingList4x4;
    std::vector<int32_t> UseDefaultScalingMatrix4x4Flag;
    std::vector<std::vector<int32_t>> ScalingList8x8;
    std::vector<int32_t> UseDefaultScalingMatrix8x8Flag;
    int32_t second_chroma_qp_index_offset;
};

/**
 * @sa  ISO 14496/10(2020) - 7.3.3 Slice header syntax
 */
class H264SliceHeaderSyntax
{
public:
    using ptr = std::shared_ptr<H264SliceHeaderSyntax>;
public:
    H264SliceHeaderSyntax();
    ~H264SliceHeaderSyntax() = default;
public:
    uint32_t  first_mb_in_slice;
    uint32_t  slice_type;
    uint32_t  pic_parameter_set_id;
    uint8_t   colour_plane_id;
    uint64_t  frame_num;
    uint8_t   field_pic_flag;
    uint8_t   bottom_field_flag;
    uint32_t  idr_pic_id;
    uint32_t  pic_order_cnt_lsb;
    int32_t   delta_pic_order_cnt_bottom;
    int32_t   delta_pic_order_cnt[2];
    int32_t   redundant_pic_cnt;
    uint8_t   direct_spatial_mv_pred_flag;
    uint8_t   num_ref_idx_active_override_flag;
    uint32_t  num_ref_idx_l0_active_minus1;
    uint32_t  num_ref_idx_l1_active_minus1;
    uint32_t  cabac_init_idc;
    int32_t   slice_qp_delta;
    uint8_t   sp_for_switch_flag;
    int32_t   slice_qs_delta;
    uint32_t  disable_deblocking_filter_idc;
    int32_t   slice_alpha_c0_offset_div2;
    int32_t   slice_beta_offset_div2;
    uint32_t  slice_group_change_cycle;
    H264ReferencePictureListModificationSyntax::ptr rplm;
    H264PredictionWeightTableSyntax::ptr pwt;
    H264DecodedReferencePictureMarkingSyntax::ptr drpm;
public:
    uint16_t slice_data_bit_offset;
};

/**
 * @sa ISO 14496/10(2020) - 7.3.1 NAL unit syntax
 */
class H264NalSyntax
{
public:
    using ptr = std::shared_ptr<H264NalSyntax>;
public:
    H264NalSyntax();
    ~H264NalSyntax() = default;
public:
    uint8_t  nal_ref_idc;
    uint8_t  nal_unit_type;
    uint8_t  svc_extension_flag;
    H264NalSvcSyntax::ptr svc;
    uint8_t  avc_3d_extension_flag;
    H264Nal3dAvcSyntax::ptr avc;
    H264NalMvcSyntax::ptr mvc;
    int8_t   emulation_prevention_three_byte;
    H264SpsSyntax::ptr          sps;
    H264PpsSyntax::ptr          pps;
    H264SeiSyntax::ptr          sei;
    H264SliceHeaderSyntax::ptr  slice;
};

class H264ContextSyntax
{
public:
    using ptr = std::shared_ptr<H264ContextSyntax>;
public:
    H264ContextSyntax();
    ~H264ContextSyntax() = default;
public:
    H264NalSyntax::ptr nal;
    std::unordered_map<int32_t, H264SpsSyntax::ptr> spsSet;
    std::unordered_map<int32_t, H264PpsSyntax::ptr> ppsSet;
    H264SpsSyntax::ptr sps;
    H264PpsSyntax::ptr pps;
};

class H264RplcContext
{
public:
    using ptr = std::shared_ptr<H264RplcContext>;
public:
    H264RplcContext() = default;
    ~H264RplcContext() = default;
public:
    uint64_t  FrameNum;
    uint64_t  LongTermFrameIdx;
public:
    uint64_t  PicNum;
    uint64_t  LongTermPicNum;
};

/**
 * @sa ISO 14496/10(2020) - 8.2.5 Decoded reference picture marking process
 */
class H264DrpmContext
{
public:
    using ptr = std::shared_ptr<H264DrpmContext>;
public:
    H264DrpmContext() = default;
    ~H264DrpmContext() = default;
public:
    bool    unused_for_reference;
    bool    used_for_short_term_reference;
    bool    used_for_long_term_reference;
    int8_t  MaxLongTermFrameIdx;
    int8_t  LongTermFrameIdx;
};

} // namespace Codec
} // namespace Mmp