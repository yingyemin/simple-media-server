#include "H26xUltis.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <cassert>

#include "H264Common.h"

namespace Mmp
{
namespace Codec
{


/**
 * @sa ITU-T H.265 (2021) - Table 6-1
 */
static void GetH265SubWidthCAndSubHeightC(H265SpsSyntax::ptr sps, uint8_t& SubWidthC, uint8_t& SubHeightC)
{
    if (sps->chroma_format_idc == 0 && sps->separate_colour_plane_flag == 0)
    {
        SubWidthC = 1;
        SubHeightC = 1;
    }
    else if (sps->chroma_format_idc == 1 && sps->separate_colour_plane_flag == 0)
    {
        SubWidthC = 2;
        SubHeightC = 2;
    }
    else if (sps->chroma_format_idc == 2 && sps->separate_colour_plane_flag == 0)
    {
        SubWidthC = 2;
        SubHeightC= 1;
    }
    else if (sps->chroma_format_idc == 3 && sps->separate_colour_plane_flag == 0)
    {
        SubWidthC = 1;
        SubHeightC = 1;
    }
    else if (sps->chroma_format_idc == 3 && sps->separate_colour_plane_flag == 1)
    {
        SubWidthC = 1;
        SubHeightC = 1;
    }
    else
    {
        assert(false);
    }
}

std::string H264NaluTypeToStr(uint8_t nal_unit_type)
{
    switch (nal_unit_type)
    {
        case H264NaluType::MMP_H264_NALU_TYPE_NULL: return "NULL";
        case H264NaluType::MMP_H264_NALU_TYPE_SLICE: return "SLICE";
        case H264NaluType::MMP_H264_NALU_TYPE_DPA: return "DPA";
        case H264NaluType::MMP_H264_NALU_TYPE_DPB: return "DPB";
        case H264NaluType::MMP_H264_NALU_TYPE_DPC: return "DPC";
        case H264NaluType::MMP_H264_NALU_TYPE_IDR: return "IDR";
        case H264NaluType::MMP_H264_NALU_TYPE_SEI: return "SEI";
        case H264NaluType::MMP_H264_NALU_TYPE_SPS: return "SPS";
        case H264NaluType::MMP_H264_NALU_TYPE_PPS: return "PPS";
        case H264NaluType::MMP_H264_NALU_TYPE_AUD: return "AUD";
        case H264NaluType::MMP_H264_NALU_TYPE_EOSEQ: return "EOSEQ";
        case H264NaluType::MMP_H264_NALU_TYPE_EOSTREAM: return "EOSTREAM";
        case H264NaluType::MMP_H264_NALU_TYPE_FILL: return "FILL";
        case H264NaluType::MMP_H264_NALU_TYPE_SPSEXT: return "SPSEXT";
        case H264NaluType::MMP_H264_NALU_TYPE_PREFIX: return "PREFIX";
        case H264NaluType::MMP_H264_NALU_TYPE_SUB_SPS: return "SUB_SPS";
        case H264NaluType::MMP_H264_NALU_TYPE_SLICE_AUX: return "SLICE_AUX";
        case H264NaluType::MMP_H264_NALU_TYPE_SLC_EXT: return "SLC_EXT";
        case H264NaluType::MMP_H264_NALU_TYPE_VDRD: return "VDRD";
        default: return "UNKNOWN";
    }
}

std::string H264SliceTypeToStr(uint8_t slice_type)
{
    switch (slice_type)
    {
        case H264SliceType::MMP_H264_P_SLICE: return "P";
        case H264SliceType::MMP_H264_B_SLICE: return "B";
        case H264SliceType::MMP_H264_I_SLICE: return "I";
        case H264SliceType::MMP_H264_SP_SLICE: return "SP";
        case H264SliceType::MMP_H264_SI_SLICE: return "SI";
        default: return "?";
    }
}

void FillH265SpsContext(H265SpsSyntax::ptr sps)
{
    sps->context = std::make_shared<H265SpsContext>();
    H265SpsContext::ptr context = sps->context;

    uint8_t SubWidthC = 1, SubHeightC = 1;

    GetH265SubWidthCAndSubHeightC(sps, SubWidthC, SubHeightC);

    context->BitDepthY = 8 + sps->bit_depth_luma_minus8;
    context->QpBdOffsetY = 6 * sps->bit_depth_luma_minus8;

    context->BitDepthC = 8 + sps->bit_depth_chroma_minus8;
    context->QpBdOffsetC = 6 * sps->bit_depth_chroma_minus8;

    context->MaxPicOrderCntLsb = 1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);

    // SpsMaxLatencyPictures
    {
        size_t num = sps->sps_max_num_reorder_pics.size();
        context->SpsMaxLatencyPictures.resize(num);
        for (size_t i=0; i<num; i++)
        {
            context->SpsMaxLatencyPictures[i] = sps->sps_max_num_reorder_pics[i] + sps->sps_max_latency_increase_plus1[i] - 1;
        }
    }

    context->MinCbLog2SizeY = sps->log2_min_luma_coding_block_size_minus3 + 3;
    context->CtbLog2SizeY = context->MinCbLog2SizeY + sps->log2_diff_max_min_luma_coding_block_size;
    context->MinCbSizeY = 1 << context->MinCbLog2SizeY;
    context->CtbSizeY = 1 << context->CtbLog2SizeY;
    context->PicWidthInMinCbsY = sps->pic_width_in_luma_samples / context->MinCbSizeY;
    context->PicWidthInCtbsY = std::ceil(sps->pic_width_in_luma_samples / context->CtbSizeY);
    context->PicHeightInMinCbsY = sps->pic_height_in_luma_samples / context->MinCbSizeY;
    context->PicHeightInCtbsY = std::ceil(sps-> pic_height_in_luma_samples / context->CtbSizeY);
    context->PicSizeInMinCbsY = context->PicWidthInMinCbsY * context->PicHeightInMinCbsY;
    context->PicSizeInCtbsY = context->PicWidthInCtbsY * context->PicHeightInCtbsY;
    context->PicWidthInSamplesC = sps->pic_width_in_luma_samples / SubWidthC;
    context->PicHeightInSamplesC = sps->pic_height_in_luma_samples / SubHeightC;

    context->CtbWidthC = context->CtbSizeY / SubWidthC;
    context->CtbHeightC = context->CtbSizeY / SubHeightC;
}


/**
 * @sa ISO 14496/10(2020) - Table 6-1
 */
static void GetH264SubWidthCAndSubHeightC(H264SpsSyntax::ptr sps, uint8_t& SubWidthC, uint8_t& SubHeightC)
{
    SubWidthC = 1, SubHeightC = 1;
    if (sps->chroma_format_idc == 0 && sps->separate_colour_plane_flag == 0)
    {
        assert(false);
    }
    else if (sps->chroma_format_idc == 1 && sps->separate_colour_plane_flag == 0)
    {
        SubWidthC = 2;
        SubHeightC = 2;
    }
    else if (sps->chroma_format_idc == 2 && sps->separate_colour_plane_flag == 0)
    {
        SubWidthC = 2;
        SubHeightC = 1;
    }
    else if (sps->chroma_format_idc == 3 && sps->separate_colour_plane_flag == 0)
    {
        // SubWidthC = 1;
        // SubHeightC = 1;
    }
    else if (sps->chroma_format_idc == 3 && sps->separate_colour_plane_flag == 1)
    {
        assert(false);
    }
    else
    {
        assert(false);
    }
}

void FillH264SpsContext(H264SpsSyntax::ptr sps)
{
    uint8_t SubWidthC, SubHeightC;
    uint8_t MbWidthC, MbHeightC;

    GetH264SubWidthCAndSubHeightC(sps, SubWidthC, SubHeightC);
    MbWidthC = 16 / SubWidthC; // (6-1)
    MbHeightC = 16 / SubHeightC; // (6-2)

    sps->context = std::make_shared<H264SpsContext>();
    H264SpsContext::ptr context = sps->context;

    context->BitDepthY = 8 + sps->bit_depth_luma_minus8;
    context->QpBdOffsetY = 6 * sps->bit_depth_luma_minus8;

    context->BitDepthC = 8 + sps->bit_depth_chroma_minus8;
    context->QpBdOffsetC = 6 * sps->bit_depth_chroma_minus8;

    context->RawMbBits = 256 * context->BitDepthY + 2 * MbWidthC * MbHeightC * context->BitDepthC;

    context->MaxFrameNum = 1 << (sps->log2_max_frame_num_minus4 + 4);

    context->MaxPicOrderCntLsb = 1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);

    context->ExpectedDeltaPerPicOrderCntCycle = 0;
    for (uint32_t i=0; i<sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
    {
        context->ExpectedDeltaPerPicOrderCntCycle += sps->offset_for_ref_frame[i];
    }

    context->PicWidthInMbs = sps->pic_width_in_mbs_minus1 + 1;
    context->PicWidthInSamplesL = context->PicWidthInMbs * 16;
    context->PicWidthInSamplesC = context->PicWidthInMbs * MbWidthC;

    context->PicHeightInMapUnits = sps->pic_height_in_map_units_minus1 + 1;
    context->PicSizeInMapUnits = context->PicWidthInMbs * context->PicHeightInMapUnits;

    context->FrameHeightInMbs = (2 - sps->frame_mbs_only_flag) * context->PicHeightInMapUnits;

    uint32_t ChromaArrayType = sps->separate_colour_plane_flag == 1 ? 0 : sps->chroma_format_idc;
    if (ChromaArrayType == 0)
    {
        context->CropUnitX = 1;
        context->CropUnitY = 2 - sps->frame_mbs_only_flag;
    }
    else
    {
        context->CropUnitX = SubWidthC;
        context->CropUnitY = SubHeightC * (2 - sps->frame_mbs_only_flag);
    }
}

} // namespace Codec
} // namespace Mmp