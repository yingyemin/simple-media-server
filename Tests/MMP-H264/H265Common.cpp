#include "H265Common.h"
#include <cstdint>
#include <cstring>

namespace Mmp
{
namespace Codec
{

H265StRefPicSetSyntax::H265StRefPicSetSyntax()
{
    inter_ref_pic_set_prediction_flag = 0;
    delta_idx_minus1 = 0;
    delta_rps_sign = 0;
    abs_delta_rps_minus1 = 0;
    num_negative_pics = 0;
    num_positive_pics = 0;
}

H265SpsRangeSyntax::H265SpsRangeSyntax()
{
    transform_skip_rotation_enabled_flag = 0;
    transform_skip_context_enabled_flag = 0;
    implicit_rdpcm_enabled_flag = 0;
    explicit_rdpcm_enabled_flag = 0;
    extended_precision_processing_flag = 0;
    intra_smoothing_disabled_flag = 0;
    high_precision_offsets_enabled_flag = 0;
    persistent_rice_adaptation_enabled_flag = 0;
    cabac_bypass_alignment_enabled_flag = 0;
}

H265ColourMappingTable::H265ColourMappingTable()
{
    num_cm_ref_layers_minus1 = 0;
    cm_octant_depth = 0;
    cm_y_part_num_log2 = 0;
    luma_bit_depth_cm_input_minus8 = 0;
    chroma_bit_depth_cm_input_minus8 = 0;
    luma_bit_depth_cm_output_minus8 = 0;
    chroma_bit_depth_cm_output_minus8 = 0;
    cm_res_quant_bits = 0;
    cm_delta_flc_bits_minus1 = 0;
    cm_adapt_threshold_u_delta = 0;
    cm_adapt_threshold_v_delta = 0;
}

H265PpsMultilayerSyntax::H265PpsMultilayerSyntax()
{
    poc_reset_info_present_flag = 0;
    pps_infer_scaling_list_flag = 0;
    pps_scaling_list_ref_layer_id = 0;
    num_ref_loc_offsets = 0;
    colour_mapping_enabled_flag = 0;
    memset(scaled_ref_layer_left_offset, 0, sizeof(int32_t) * (1<<6u));
    memset(scaled_ref_layer_top_offset, 0, sizeof(int32_t) * (1<<6u));
    memset(scaled_ref_layer_right_offset, 0, sizeof(int32_t) * (1<<6u));
    memset(scaled_ref_layer_bottom_offset, 0, sizeof(int32_t) * (1<<6u));
    memset(ref_region_left_offset, 0, sizeof(int32_t) * (1<<6u));
    memset(ref_region_top_offset, 0, sizeof(int32_t) * (1<<6u));
    memset(ref_region_right_offset, 0, sizeof(int32_t) * (1<<6u));
    memset(ref_region_bottom_offset, 0, sizeof(int32_t) * (1<<6u));
    memset(ref_region_bottom_offset, 0, sizeof(uint32_t) * (1<<6u));
    memset(phase_hor_luma, 0, sizeof(uint32_t) * (1<<6u));
    memset(phase_ver_luma, 0, sizeof(uint32_t) * (1<<6u));
    memset(phase_hor_chroma_plus8, 0, sizeof(uint32_t) * (1<<6u));
    memset(phase_ver_chroma_plus8, 0, sizeof(uint32_t) * (1<<6u));
}

H265Sps3DSyntax::H265Sps3DSyntax()
{
    iv_di_mc_enabled_flag[0] = 0;
    iv_di_mc_enabled_flag[1] = 0;
    iv_mv_scal_enabled_flag[0] = 0;
    iv_mv_scal_enabled_flag[1] = 0;
    log2_ivmc_sub_pb_size_minus3 = 0;
    iv_res_pred_enabled_flag = 0;
    depth_ref_enabled_flag = 0;
    vsp_mc_enabled_flag = 0;
    dbbp_enabled_flag = 0;
    tex_mc_enabled_flag = 0;
    log2_texmc_sub_pb_size_minus3 = 0;
    intra_contour_enabled_flag = 0;
    intra_dc_only_wedge_enabled_flag = 0;
    cqt_cu_part_pred_enabled_flag = 0;
    inter_dc_only_enabled_flag = 0;
    skip_intra_enabled_flag = 0;
}

H265SpsSccSyntax::H265SpsSccSyntax()
{
    sps_curr_pic_ref_enabled_flag = 0;
    palette_mode_enabled_flag = 0;
    palette_max_size = 0;
    delta_palette_max_predictor_size = 0;
    sps_palette_predictor_initializers_present_flag = 0;
    sps_num_palette_predictor_initializers_minus1 = 0;
    motion_vector_resolution_control_idc = 0;
    intra_boundary_filtering_disabled_flag = 0;
}

H265PpsRangeSyntax::H265PpsRangeSyntax()
{
    log2_max_transform_skip_block_size_minus2 = 0;
    cross_component_prediction_enabled_flag = 0;
    chroma_qp_offset_list_enabled_flag = 0;
    diff_cu_chroma_qp_offset_depth = 0;
    chroma_qp_offset_list_len_minus1 = 0;
    log2_sao_offset_scale_luma = 0;
    log2_sao_offset_scale_chroma = 0;  
}

H265PpsSccSyntax::H265PpsSccSyntax()
{
    pps_curr_pic_ref_enabled_flag = 0;
    residual_adaptive_colour_transform_enabled_flag = 0;
    pps_slice_act_qp_offsets_present_flag = 0;
    pps_act_y_qp_offset_plus5 = 0;
    pps_act_cb_qp_offset_plus5 = 0;
    pps_act_cr_qp_offset_plus3 = 0;
    pps_palette_predictor_initializers_present_flag = 0;
    pps_num_palette_predictor_initializers = 0;
    monochrome_palette_flag = 0;
    luma_bit_depth_entry_minus8 = 0;
    chroma_bit_depth_entry_minus8 = 0; 
}

H265DeltaDltSyntax::H265DeltaDltSyntax()
{
    num_val_delta_dlt = 0;
    max_diff = 0;
    min_diff_minus1 = 0;
    delta_dlt_val0 = 0;
}

H265Pps3dSyntax::H265Pps3dSyntax()
{
    dlts_present_flag = 0;
    pps_depth_layers_minus1 = 0;
    pps_bit_depth_for_depth_layers_minus8 = 0;
}

H265HrdSyntax::H265HrdSyntax()
{
    nal_hrd_parameters_present_flag = 0;
    vcl_hrd_parameters_present_flag = 0;
    sub_pic_hrd_params_present_flag = 0;
    tick_divisor_minus2 = 0;
    du_cpb_removal_delay_increment_length_minus1 = 0;
    sub_pic_cpb_params_in_pic_timing_sei_flag = 0;
    dpb_output_delay_du_length_minus1 = 0;
    bit_rate_scale = 0;
    cpb_size_scale = 0;
    cpb_size_du_scale = 0;
    initial_cpb_removal_delay_length_minus1 = 0;
    au_cpb_removal_delay_length_minus1 = 0;
    dpb_output_delay_length_minus1 = 0;
}

H265VuiSyntax::H265VuiSyntax()
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
    matrix_coeffs = 0;
    chroma_loc_info_present_flag = 0;
    chroma_sample_loc_type_top_field = 0;
    chroma_sample_loc_type_bottom_field = 0;
    neutral_chroma_indication_flag = 0;
    field_seq_flag = 0;
    frame_field_info_present_flag = 0;
    default_display_window_flag = 0;
    def_disp_win_left_offset = 0;
    def_disp_win_right_offset = 0;
    def_disp_win_top_offset = 0;
    def_disp_win_bottom_offset = 0;
    vui_timing_info_present_flag = 0;
    vui_num_units_in_tick = 0;
    vui_time_scale = 0;
    vui_poc_proportional_to_timing_flag = 0;
    vui_num_ticks_poc_diff_one_minus1 = 0;
    vui_hrd_parameters_present_flag = 0;
    bitstream_restriction_flag = 0;
    tiles_fixed_structure_flag = 0;
    motion_vectors_over_pic_boundaries_flag = 0;
    restricted_ref_pic_lists_flag = 0;
    min_spatial_segmentation_idc = 0;
    max_bytes_per_pic_denom = 0;
    max_bits_per_min_cu_denom = 0;
    log2_max_mv_length_horizontal = 0;
    log2_max_mv_length_vertical = 0;
}

H265PTLSyntax::H265PTLSyntax()
{
    general_profile_space = 0;
    general_tier_flag = 0;
    general_profile_idc = 0;
    memset(general_profile_compatibility_flag, 0, sizeof(uint8_t) * 32);
    general_progressive_source_flag = 0;
    general_interlaced_source_flag = 0;
    general_non_packed_constraint_flag = 0;
    general_frame_only_constraint_flag = 0;
    general_max_12bit_constraint_flag = 0;
    general_max_10bit_constraint_flag = 0;
    general_max_8bit_constraint_flag = 0;
    general_max_422chroma_constraint_flag = 0;
    general_max_420chroma_constraint_flag = 0;
    general_max_monochrome_constraint_flag = 0;
    general_intra_constraint_flag = 0;
    general_one_picture_only_constraint_flag = 0;
    general_lower_bit_rate_constraint_flag = 0;
    general_max_14bit_constraint_flag = 0;
    general_reserved_zero_33bits = 0;
    general_reserved_zero_34bits = 0;
    general_reserved_zero_7bits = 0;
    general_reserved_zero_35bits = 0;
    general_reserved_zero_43bits = 0;
    general_inbld_flag = 0;
    general_reserved_zero_bit = 0;
    general_level_idc = 0;
}

H265VPSSyntax::H265VPSSyntax()
{
    vps_video_parameter_set_id = 0;
    vps_base_layer_internal_flag = 0;
    vps_base_layer_available_flag = 0;
    vps_max_layers_minus1 = 0;
    vps_max_sub_layers_minus1 = 0;
    vps_temporal_id_nesting_flag = 0;
    vps_reserved_0xffff_16bits = 0;
    vps_sub_layer_ordering_info_present_flag = 0;
    vps_max_layer_id = 0;
    vps_num_layer_sets_minus1 = 0;
    vps_timing_info_present_flag = 0;
    vps_num_units_in_tick = 0;
    vps_time_scale = 0;
    vps_poc_proportional_to_timing_flag = 0;
    vps_num_ticks_poc_diff_one_minus1 = 0;
    vps_num_hrd_parameters = 0;
    vps_extension_flag = 0;
    vps_extension_data_flag = 0;
}

H265SpsContext::H265SpsContext()
{
    BitDepthY = 0;  
    QpBdOffsetY = 0;
    BitDepthC = 0;  
    QpBdOffsetC = 0;

    MaxPicOrderCntLsb = 0;

    MinCbLog2SizeY = 0;     
    CtbLog2SizeY = 0;       
    MinCbSizeY = 0;         
    CtbSizeY = 0;           
    PicWidthInMinCbsY = 0;  
    PicWidthInCtbsY = 0;    
    PicHeightInMinCbsY = 0; 
    PicHeightInCtbsY = 0;   
    PicSizeInMinCbsY = 0;   
    PicSizeInCtbsY = 0;     
    PicSizeInSamplesY = 0;  
    PicWidthInSamplesC = 0; 
    PicHeightInSamplesC = 0;

    CtbWidthC = 0; 
    CtbHeightC = 0;

    PcmBitDepthY = 0;
    PcmBitDepthC = 0;
}

H265SpsSyntax::H265SpsSyntax()
{
    sps_video_parameter_set_id = 0;
    sps_max_sub_layers_minus1 = 0;
    sps_temporal_id_nesting_flag = 0;
    sps_seq_parameter_set_id = 0;
    chroma_format_idc = 0;
    separate_colour_plane_flag = 0;
    pic_width_in_luma_samples = 0;
    pic_height_in_luma_samples = 0;
    conformance_window_flag = 0;
    conf_win_left_offset = 0;
    conf_win_right_offset = 0;
    conf_win_top_offset = 0;
    conf_win_bottom_offset = 0;
    bit_depth_luma_minus8 = 0;
    bit_depth_chroma_minus8 = 0;
    log2_max_pic_order_cnt_lsb_minus4 = 0;
    sps_sub_layer_ordering_info_present_flag = 0;
    log2_min_luma_coding_block_size_minus3 = 0;
    log2_diff_max_min_luma_coding_block_size = 0;
    log2_min_luma_transform_block_size_minus2 = 0;
    log2_diff_max_min_luma_transform_block_size = 0;
    max_transform_hierarchy_depth_inter = 0;
    max_transform_hierarchy_depth_intra = 0;
    scaling_list_enabled_flag = 0;
    sps_scaling_list_data_present_flag = 0;
    amp_enabled_flag = 0;
    sample_adaptive_offset_enabled_flag = 0;
    pcm_enabled_flag = 0;
    pcm_sample_bit_depth_luma_minus1 = 0;
    pcm_sample_bit_depth_chroma_minus1 = 0;
    log2_min_pcm_luma_coding_block_size_minus3 = 0;
    log2_diff_max_min_pcm_luma_coding_block_size = 0;
    pcm_loop_filter_disabled_flag = 0;
    num_short_term_ref_pic_sets = 0;
    long_term_ref_pics_present_flag = 0;
    num_long_term_ref_pics_sps = 0;
    sps_temporal_mvp_enabled_flag = 0;
    strong_intra_smoothing_enabled_flag = 0;
    vui_parameters_present_flag = 0;
    sps_extension_present_flag = 0;
    sps_range_extension_flag = 0;
    sps_multilayer_extension_flag = 0;
    sps_3d_extension_flag = 0;
    sps_scc_extension_flag = 0;
    sps_extension_4bits = 0;
    sps_extension_data_flag = 0;
}

H265PpsSyntax::H265PpsSyntax()
{
    pps_pic_parameter_set_id = 0;
    pps_seq_parameter_set_id = 0;
    dependent_slice_segments_enabled_flag = 0;
    output_flag_present_flag = 0;
    num_extra_slice_header_bits = 0;
    sign_data_hiding_enabled_flag = 0;
    cabac_init_present_flag = 0;
    num_ref_idx_l0_default_active_minus1 = 0;
    num_ref_idx_l1_default_active_minus1 = 0;
    init_qp_minus26 = 0;
    constrained_intra_pred_flag = 0;
    transform_skip_enabled_flag = 0;
    cu_qp_delta_enabled_flag = 0;
    diff_cu_qp_delta_depth = 0;
    pps_cb_qp_offset = 0;
    pps_cr_qp_offset = 0;
    pps_slice_chroma_qp_offsets_present_flag  = 0;
    weighted_pred_flag = 0;
    weighted_bipred_flag = 0;
    transquant_bypass_enabled_flag = 0;
    tiles_enabled_flag = 0;
    entropy_coding_sync_enabled_flag = 0;
    num_tile_columns_minus1 = 0;
    num_tile_rows_minus1 = 0;
    uniform_spacing_flag = 0;
    loop_filter_across_tiles_enabled_flag = 0;
    pps_loop_filter_across_slices_enabled_flag = 0;
    deblocking_filter_control_present_flag = 0;
    deblocking_filter_override_enabled_flag = 0;
    pps_deblocking_filter_disabled_flag = 0;
    pps_beta_offset_div2 = 0;
    pps_tc_offset_div2 = 0;
    pps_scaling_list_data_present_flag = 0;
    lists_modification_present_flag = 0;
    log2_parallel_merge_level_minus2 = 0;
    slice_segment_header_extension_present_flag = 0;
    pps_extension_present_flag = 0;
    pps_range_extension_flag = 0;
    pps_multilayer_extension_flag = 0;
    pps_3d_extension_flag = 0;
    pps_scc_extension_flag = 0;
    pps_extension_4bits = 0;
    pps_extension_data_flag = 0;
}

H265PpsMultilayerExtensionSyntax::H265PpsMultilayerExtensionSyntax()
{
    poc_reset_info_present_flag = 0;
    pps_infer_scaling_list_flag = 0;
    pps_scaling_list_ref_layer_id = 0;
    num_ref_loc_offsets = 0;
    memset(scaled_ref_layer_left_offset, 0, sizeof(int32_t) * (1<<6u));
    memset(scaled_ref_layer_top_offset, 0, sizeof(int32_t) * (1<<6u));
    memset(scaled_ref_layer_right_offset, 0, sizeof(int32_t) * (1<<6u));
    memset(scaled_ref_layer_bottom_offset, 0, sizeof(int32_t) * (1<<6u));
    memset(ref_region_left_offset, 0, sizeof(int32_t) * (1<<6u));
    memset(ref_region_top_offset, 0, sizeof(int32_t) * (1<<6u));
    memset(ref_region_right_offset, 0, sizeof(int32_t) * (1<<6u));
    memset(ref_region_bottom_offset, 0, sizeof(int32_t) * (1<<6u));
    memset(phase_hor_luma, 0, sizeof(int32_t) * (1<<6u));
    memset(phase_ver_luma, 0, sizeof(int32_t) * (1<<6u));
    memset(phase_ver_chroma_plus8, 0, sizeof(int32_t) * (1<<6u));
    memset(phase_ver_chroma_plus8, 0, sizeof(int32_t) * (1<<6u));
}

H265RefPicListsModificationSyntax::H265RefPicListsModificationSyntax()
{
    ref_pic_list_modification_flag_l0 = 0;
    ref_pic_list_modification_flag_l1 = 0;
}

H265PredWeightTableSyntax::H265PredWeightTableSyntax()
{
    luma_log2_weight_denom = 0;
    delta_chroma_log2_weight_denom = 0;
}

H265SliceHeaderSyntax::H265SliceHeaderSyntax()
{
    first_slice_segment_in_pic_flag = 0;
    no_output_of_prior_pics_flag = 0;
    slice_pic_parameter_set_id = 0;
    dependent_slice_segment_flag = 0;
    slice_segment_address = 0;
    slice_type = 0;
    pic_output_flag = 0;
    colour_plane_id = 0;
    slice_pic_order_cnt_lsb = 0;
    short_term_ref_pic_set_sps_flag = 0;
    short_term_ref_pic_set_idx = 0;
    num_long_term_sps = 0;
    num_long_term_pics = 0;
    slice_temporal_mvp_enabled_flag = 0;
    slice_sao_luma_flag = 0;
    slice_sao_chroma_flag = 0;
    num_ref_idx_active_override_flag = 0;
    num_ref_idx_l0_active_minus1 = 0;
    num_ref_idx_l1_active_minus1 = 0;
    mvd_l1_zero_flag = 0;
    cabac_init_flag = 0;
    collocated_from_l0_flag = 0;
    collocated_ref_idx = 0;
    five_minus_max_num_merge_cand = 0;
    use_integer_mv_flag = 0;
    slice_qp_delta = 0;
    slice_cb_qp_offset = 0;
    slice_cr_qp_offset = 0;
    slice_act_y_qp_offset = 0;
    slice_act_cb_qp_offset = 0;
    slice_act_cr_qp_offset = 0;
    cu_chroma_qp_offset_enabled_flag = 0;
    deblocking_filter_override_flag = 0;
    slice_deblocking_filter_disabled_flag = 0;
    slice_beta_offset_div2 = 0;
    slice_tc_offset_div2 = 0;
    slice_loop_filter_across_slices_enabled_flag = 0;
    num_entry_point_offsets = 0;
    offset_len_minus1 = 0;
    slice_segment_header_extension_length = 0;
}

H265SeiPicTimingSyntax::H265SeiPicTimingSyntax()
{
    pic_struct = 0;
    source_scan_type = 0;
    duplicate_flag = 0;
    au_cpb_removal_delay_minus1 = 0;
    pic_dpb_output_delay = 0;
    pic_dpb_output_du_delay = 0;
    num_decoding_units_minus1 = 0;
    du_common_cpb_removal_delay_flag = 0;
    du_common_cpb_removal_delay_increment_minus1 = 0;
}

H265SeiRecoveryPointSyntax::H265SeiRecoveryPointSyntax()
{
    recovery_poc_cnt = 0;
    exact_match_flag = 0;
    broken_link_flag = 0;
}

H265SeiDecodedPictureHashSyntax::H265SeiDecodedPictureHashSyntax()
{
    hash_type = 0;
}

H265SeiActiveParameterSetsSyntax::H265SeiActiveParameterSetsSyntax()
{
    active_video_parameter_set_id = 0;
    self_contained_cvs_flag = 0;
    no_parameter_set_update_flag = 0;
    num_sps_ids_minus1 = 0;
}

H265SeiTimeCodeSyntax::H265SeiTimeCodeSyntax()
{
    num_clock_ts = 0;
}

H265MasteringDisplayColourVolumeSyntax::H265MasteringDisplayColourVolumeSyntax()
{
    memset(display_primaries_x, 0, sizeof(uint16_t) * 3);
    memset(display_primaries_y, 0, sizeof(uint16_t) * 3);
    white_point_x = 0;
    white_point_y = 0;
    max_display_mastering_luminance = 0;
    min_display_mastering_luminance = 0;
}

H265ContentLightLevelInformationSyntax::H265ContentLightLevelInformationSyntax()
{
    max_content_light_level = 0;
    max_pic_average_light_level = 0;
}

H265ContentColourVolumeSyntax::H265ContentColourVolumeSyntax()
{
    ccv_cancel_flag = 0;
    ccv_persistence_flag = 0;
    ccv_primaries_present_flag = 0;
    ccv_min_luminance_value_present_flag = 0;
    ccv_max_luminance_value_present_flag = 0;
    ccv_avg_luminance_value_present_flag = 0;
    ccv_reserved_zero_2bits = 0;
    memset(ccv_primaries_x, 0, sizeof(int32_t) * 3);
    memset(ccv_primaries_y, 0, sizeof(int32_t) * 3);
    ccv_min_luminance_value = 0;
    ccv_max_luminance_value = 0;
    ccv_avg_luminance_value = 0;
}

H265SeiMessageSyntax::H265SeiMessageSyntax()
{
    payloadType = 0;
    payloadSize = 0;
}

H265NalUnitHeaderSyntax::H265NalUnitHeaderSyntax()
{
    forbidden_zero_bit = 0;
    nal_unit_type = 0;
    nuh_layer_id = 0;
    nuh_temporal_id_plus1 = 0;
}

H265ContextSyntax::H265ContextSyntax()
{
    
}

} // names1ace Codec
} // namespace Mmp