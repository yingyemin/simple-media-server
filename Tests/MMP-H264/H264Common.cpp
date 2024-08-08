#include "H264Common.h"

#include <memory.h>

namespace Mmp
{
namespace Codec
{

H264HrdSyntax::H264HrdSyntax()
{
    cpb_cnt_minus1 = 0;
    bit_rate_scale = 0;
    cpb_size_scale = 0;
    initial_cpb_removal_delay_length_minus1 = 0;
    cpb_removal_delay_length_minus1 = 0;
    dpb_output_delay_length_minus1 = 0;
    time_offset_length = 0;
}

H264VuiSyntax::H264VuiSyntax()
{
    aspect_ratio_info_present_flag = 0;
    aspect_ratio_idc = 0;
    sar_width = 0;
    sar_height = 0;
    overscan_info_present_flag = 0;
    overscan_appropriate_flag = 0;
    video_signal_type_present_flag = 0;
    video_format = 0;
    video_full_range_flag = 0;
    colour_description_present_flag = 0;
    colour_primaries = 0;
    transfer_characteristics = 0;
    matrix_coefficients = 0;
    chroma_location_info_present_flag = 0;
    chroma_sample_loc_type_top_field = 0;
    chroma_sample_loc_type_bottom_field = 0;
    timing_info_present_flag = 0;
    num_units_in_tick = 0;
    time_scale = 0;
    fixed_frame_rate_flag = 0;
    nal_hrd_parameters_present_flag = 0;
    nal_hrd_parameters = nullptr;
    vcl_hrd_parameters_present_flag = 0;
    vcl_hrd_parameters = nullptr;
    low_delay_hrd_flag = 0;
    pic_struct_present_flag = 0;
    bitstream_restriction_flag = 0;
    motion_vectors_over_pic_boundaries_flag = 0;
    max_bytes_per_pic_denom = 0;
    max_bits_per_mb_denom = 0;
    log2_max_mv_length_vertical = 0;
    log2_max_mv_length_horizontal = 0;
    num_reorder_frames = 0;
    max_dec_frame_buffering = 0;
}

H264NalSvcSyntax::H264NalSvcSyntax()
{
    idr_flag = 0;
    priority_id = 0;
    no_inter_layer_pred_flag = 0;
    dependency_id = 0;
    quality_id = 0;
    temporal_id = 0;
    use_ref_base_pic_flag = 0;
    discardable_flag = 0;
    output_flag = 0;
    reserved_three_2bits = 0;
}

H264Nal3dAvcSyntax::H264Nal3dAvcSyntax()
{
    view_idx = 0;
    depth_flag = 0;
    non_idr_flag = 0;
    temporal_id = 0;
    anchor_pic_flag = 0;
    inter_view_flag = 0;
}

H264NalMvcSyntax::H264NalMvcSyntax()
{
    non_idr_flag = 0;
    priority_id = 0;
    view_id = 0;
    temporal_id = 0;
    anchor_pic_flag = 0;
    inter_view_flag = 0;
    reserved_one_bit = 0;  
}

H264SeiPictureTimingSyntax::H264SeiPictureTimingSyntax()
{
    cpb_removal_delay = 0;
    dpb_output_delay = 0;
    pic_struct = 0;   
}

H264SeiBufferPeriodSyntax::H264SeiBufferPeriodSyntax()
{
    seq_parameter_set_id = 0;
}

H264SeiUserDataRegisteredSyntax::H264SeiUserDataRegisteredSyntax()
{
    itu_t_t35_country_code = 0;
    itu_t_t35_country_code_extension_byte = 0;
}

H264SeiUserDataUnregisteredSyntax::H264SeiUserDataUnregisteredSyntax()
{
    memset(uuid_iso_iec_11578, 0, sizeof(uuid_iso_iec_11578));
}

H264SeiRecoveryPointSyntax::H264SeiRecoveryPointSyntax()
{
    recovery_frame_cnt = 0;
    exact_match_flag = 0;
    broken_link_flag = 0;
    changing_slice_group_idc = 0;
}

H264SeiFilmGrainSyntax::H264SeiFilmGrainSyntax()
{
    film_grain_characteristics_cancel_flag = 0;
    film_grain_model_id = 0;
    separate_colour_description_present_flag = 0;
    film_grain_bit_depth_luma_minus8 = 0;
    film_grain_bit_depth_chroma_minus8 = 0;
    film_grain_full_range_flag = 0;
    film_grain_colour_primaries = 0;
    film_grain_transfer_characteristics = 0;
    film_grain_matrix_coefficients = 0;
    memset(comp_model_present_flag, 0, 3);
    memset(num_intensity_intervals_minus1, 0, 3);
    memset(num_model_values_minus1, 0, 3);
    blending_mode_id = 0;
    log2_scale_factor = 0;
    film_grain_characteristics_repetition_period = 0;
}

H264ContextSyntax::H264ContextSyntax()
{
}

H264SeiFramePackingArrangementSyntax::H264SeiFramePackingArrangementSyntax()
{
    frame_packing_arrangement_id = 0;
    frame_packing_arrangement_cancel_flag = 0;
    frame_packing_arrangement_type = 0;
    quincunx_sampling_flag = 0;
    content_interpretation_type = 0;
    spatial_flipping_flag = 0;
    frame0_flipped_flag = 0;
    field_views_flag = 0;
    current_frame_is_frame0_flag = 0;
    frame0_self_contained_flag = 0;
    frame1_self_contained_flag = 0;
    frame0_grid_position_x = 0;
    frame0_grid_position_y = 0;
    frame1_grid_position_x = 0;
    frame1_grid_position_y = 0;
    frame_packing_arrangement_reserved_byte = 0;
    frame_packing_arrangement_repetition_period = 0;
    frame_packing_arrangement_extension_flag = 0;
}

H264SeiContentLigntLevelInfoSyntax::H264SeiContentLigntLevelInfoSyntax()
{
    max_content_light_level = 0;
    max_pic_average_light_level = 0;
}

H264SeiAlternativeTransferCharacteristicsSyntax::H264SeiAlternativeTransferCharacteristicsSyntax()
{
    preferred_transfer_characteristics = 0;
}

H264AmbientViewingEnvironmentSyntax::H264AmbientViewingEnvironmentSyntax()
{
    ambient_illuminance = 0;
    ambient_light_x = 0;
    ambient_light_y = 0;
}

H264SeiDisplayOrientationSyntax::H264SeiDisplayOrientationSyntax()
{
    display_orientation_cancel_flag = 0;
    hor_flip = 0;
    ver_flip = 0;
    anticlockwise_rotation = 0;
    display_orientation_repetition_period = 0;
    display_orientation_extension_flag = 0;
}

H264MasteringDisplayColourVolumeSyntax::H264MasteringDisplayColourVolumeSyntax()
{
    memset(display_primaries_x, 0, sizeof(uint16_t) * 3);
    memset(display_primaries_y, 0, sizeof(uint16_t) * 3);
    white_point_x = 0;
    white_point_y = 0;
    max_display_mastering_luminance = 0;
    min_display_mastering_luminance = 0;
}

H264SeiSyntax::H264SeiSyntax()
{
    payloadType = 0;
    payloadSize = 0;
}

H264SpsSvcSynctax::H264SpsSvcSynctax()
{
    inter_layer_deblocking_filter_control_present_flag = 0;
    extended_spatial_scalability_idc = 0;
    chroma_phase_x_plus1_flag = 0;
    chroma_phase_y_plus1 = 0;
    seq_ref_layer_chroma_phase_x_plus1_flag = 0;
    seq_ref_layer_chroma_phase_y_plus1 = 0;
    seq_scaled_ref_layer_left_offse = 0;
    seq_scaled_ref_layer_top_offset = 0;
    seq_scaled_ref_layer_right_offse = 0;
    seq_scaled_ref_layer_bottom_offset = 0;
    seq_tcoeff_level_prediction_flag = 0;
    adaptive_tcoeff_level_prediction_flag = 0;
    slice_header_restriction_flag = 0; 
}

H264SpsMvcSyntax::H264SpsMvcSyntax()
{
    num_views_minus1 = 0;
    num_level_values_signalled_minus1 = 0;
}

H264MvcVuiSyntax::H264MvcVuiSyntax()
{
    vui_mvc_num_ops_minus1 = 0;  
}

H264ReferencePictureListModificationSyntax::H264ReferencePictureListModificationSyntax()
{
    ref_pic_list_modification_flag_l0 = 0;
    ref_pic_list_modification_flag_l1 = 0;
}

H264PredictionWeightTableSyntax::H264PredictionWeightTableSyntax()
{
    luma_log2_weight_denom = 0;
    chroma_log2_weight_denom = 0;
}

H264DecodedReferencePictureMarkingSyntax::H264DecodedReferencePictureMarkingSyntax()
{
    no_output_of_prior_pics_flag = 0;
    long_term_reference_flag = 0;
    adaptive_ref_pic_marking_mode_flag = 0;
}

H264SubSpsSyntax::H264SubSpsSyntax()
{
    svc_vui_parameters_present_flag = 0;
    bit_equal_to_one = 0;
    mvc_vui_parameters_present_flag = 0;
    additional_extension2_flag = 0;
    additional_extension2_data_flag = 0;
}

H264SpsContext::H264SpsContext()
{
    BitDepthY = 0;                       
    QpBdOffsetY = 0;                     
    BitDepthC = 0;                       
    QpBdOffsetC = 0;                     
    RawMbBits = 0;                       
    MaxFrameNum = 0;                     
    MaxPicOrderCntLsb = 0;               
    ExpectedDeltaPerPicOrderCntCycle = 0;
    PicWidthInMbs = 0;                   
    PicWidthInSamplesL = 0;              
    PicWidthInSamplesC = 0;              
    PicHeightInMapUnits = 0;             
    PicSizeInMapUnits = 0;               
    FrameHeightInMbs = 0;                
    CropUnitX = 0;                       
    CropUnitY = 0;                       
}

H264SpsSyntax::H264SpsSyntax()
{
    profile_idc = 0;
    constraint_set0_flag = 0;
    constraint_set1_flag = 0;
    constraint_set2_flag = 0;
    constraint_set3_flag = 0;
    constraint_set4_flag = 0;
    constraint_set5_flag = 0;
    level_idc = 0;
    seq_parameter_set_id = 0;
    chroma_format_idc = 0;
    separate_colour_plane_flag = 0;
    bit_depth_luma_minus8 = 0;
    bit_depth_chroma_minus8 = 0;
    qpprime_y_zero_transform_bypass_flag = 0;
    seq_scaling_matrix_present_flag = 0;
    log2_max_frame_num_minus4 = 0;
    pic_order_cnt_type = 0;
    log2_max_pic_order_cnt_lsb_minus4 = 0;
    delta_pic_order_always_zero_flag = 0;
    offset_for_non_ref_pic = 0;
    offset_for_top_to_bottom_field = 0;
    num_ref_frames_in_pic_order_cnt_cycle = 0;
    max_num_ref_frames = 0;
    gaps_in_frame_num_value_allowed_flag = 0;
    pic_width_in_mbs_minus1 = 0;
    pic_height_in_map_units_minus1 = 0;
    frame_mbs_only_flag = 0;
    mb_adaptive_frame_field_flag = 0;
    direct_8x8_inference_flag = 0;
    frame_cropping_flag = 0;
    frame_crop_left_offset = 0;
    frame_crop_right_offset = 0;
    frame_crop_top_offset = 0;
    frame_crop_bottom_offset = 0;
    vui_parameters_present_flag = 0;
}

H264PpsSyntax::H264PpsSyntax()
{
    pic_parameter_set_id = 0;
    seq_parameter_set_id = 0;
    entropy_coding_mode_flag = 0;
    bottom_field_pic_order_in_frame_present_flag = 0;
    num_slice_groups_minus1 = 0;
    slice_group_map_type = 0;
    num_ref_idx_l0_default_active_minus1 = 0;
    num_ref_idx_l1_default_active_minus1 = 0;
    weighted_pred_flag = 0;
    weighted_bipred_idc = 0;
    pic_init_qp_minus26 = 0;
    pic_init_qs_minus26 = 0;
    chroma_qp_index_offset = 0;
    deblocking_filter_control_present_flag = 0;
    constrained_intra_pred_flag = 0;
    redundant_pic_cnt_present_flag = 0;
    transform_8x8_mode_flag = 0;
    pic_scaling_matrix_present_flag = 0;
    second_chroma_qp_index_offset  = 0;
}

H264SliceHeaderSyntax::H264SliceHeaderSyntax()
{
    first_mb_in_slice = 0;
    slice_type = 0;
    pic_parameter_set_id = 0;
    colour_plane_id = 0;
    frame_num = 0;
    field_pic_flag = 0;
    bottom_field_flag = 0;
    idr_pic_id = 0;
    pic_order_cnt_lsb = 0;
    delta_pic_order_cnt_bottom = 0;
    delta_pic_order_cnt[0] = 0;
    delta_pic_order_cnt[1] = 0;
    redundant_pic_cnt = 0;
    direct_spatial_mv_pred_flag = 0;
    num_ref_idx_active_override_flag = 0;
    num_ref_idx_l0_active_minus1 = 0;
    num_ref_idx_l1_active_minus1 = 0;
    cabac_init_idc = 0;
    slice_qp_delta = 0;
    sp_for_switch_flag = 0;
    slice_qs_delta = 0;
    disable_deblocking_filter_idc = 0;
    slice_alpha_c0_offset_div2 = 0;
    slice_beta_offset_div2 = 0;
    slice_group_change_cycle = 0;
    slice_data_bit_offset = 0;
}

H264NalSyntax::H264NalSyntax()
{
    nal_ref_idc = 0;
    nal_unit_type = 0;
    svc_extension_flag = 0;
    avc_3d_extension_flag = 0;
    emulation_prevention_three_byte = 0;
}

} // namespace Codec
} // namespace Mmp