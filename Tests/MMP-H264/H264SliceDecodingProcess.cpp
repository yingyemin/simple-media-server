#include "H264SliceDecodingProcess.h"

#include <cstdint>
#include <deque>
#include <sstream>
#include <cassert>
#include <iostream>
#include <algorithm>

#include "H26xUltis.h"

namespace Mmp
{
namespace Codec
{

#ifndef ENABLE_MMP_SD_DEBUG
    #define ENABLE_MMP_SD_DEBUG 0
#endif /* ENABLE_MMP_SD_DEBUG */

#if ENABLE_MMP_SD_DEBUG
#define MPP_H264_SD_LOG(fmt, ...)  do {\
                                          char buf[512] = {0};\
                                          sprintf(buf, fmt, ## __VA_ARGS__);\
                                          H26x_LOG_INFO << buf << H26x_LOG_TERMINATOR;\
                                      } while(0);
#else
#define MPP_H264_SD_LOG(fmt, ...) 
#endif /* ENABLE_MMP_SD_DEBUG */ 

// Reference: openH264(cfbd5896606b91638c8871ee91776dee31625bd5) 
//             int32_t WelsMarkAsRef (PWelsDecoderContext pCtx, PPicture pLastDec)
constexpr int64_t no_long_term_frame_indices = -1;

H264PictureContext::H264PictureContext()
{
    id = 0;
    id = 0;
    field_pic_flag = 0;
    bottom_field_flag = 0;
    pic_order_cnt_lsb = 0;
    long_term_frame_idx = 0;
    TopFieldOrderCnt = INT32_MAX;
    BottomFieldOrderCnt = INT32_MAX;
    has_memory_management_control_operation_5 = false;
    prevPicOrderCntMsb = 0;
    FrameNumOffset = 0;
    MaxFrameNum = 0;
    FrameNum = 0;
    FrameNumWrap = 0;
    PicNum = 0;
    referenceFlag = 0;
    MaxLongTermFrameIdx = 0;
    LongTermFrameIdx = 0;
    LongTermPicNum = 0;
}

static bool PictureIsSecondField(H264PictureContext::ptr picture)
{
    return picture->bottom_field_flag == 1;
}

static H264PictureContext::ptr /* complementary picture */ FindComplementaryPicture(H264PictureContext::cache pictures, H264PictureContext::ptr picture)
{
    H264PictureContext::ptr compPicture = nullptr;
    if (picture->field_pic_flag == 0)
    {
        
    }
    else if (PictureIsSecondField(picture)) // second field
    {
        for (auto& _picture : pictures)
        {
            if (_picture->PicNum /* second field */ == picture->PicNum - 1 /* first field */)
            {
                compPicture = _picture;
                break;
            }
        }
    }
    else if (picture->bottom_field_flag == 0 /* && picture->bottom_field_flag == 1 */) // first field
    {
        for (auto& _picture : pictures)
        {
            if (_picture->PicNum /* first field */ == picture->PicNum + 1 /* second field */)
            {
                compPicture = _picture;
                break;
            }
        }
    }
    return compPicture;
}

static int32_t GetPicNumX(H264SliceHeaderSyntax::ptr slice, uint32_t difference_of_pic_nums_minus1)
{
    int32_t picNumX = 0;
    {
        // The variable CurrPicNum is derived as follows:
        // - If field_pic_flag is equal to 0, CurrPicNum is set equal to frame_num.
        // - Otherwise (field_pic_flag is equal to 1), CurrPicNum is set equal to 2 * frame_num + 1
        uint64_t CurrPicNum = 0;
        if (slice->field_pic_flag == 0)
        {
            CurrPicNum = slice->frame_num;
        }
        else if (slice->field_pic_flag == 1)
        {
            // CurrPicNum = 2 * slice->frame_num + 1;
            // Hint : not support for now
            assert(false);
        }
        picNumX = (int32_t)(CurrPicNum - (difference_of_pic_nums_minus1 + 1)); // (8-39)
    }
    return picNumX;
}

static H264PictureContext::ptr FindPictureByPicNum(H264PictureContext::cache pictures, int64_t PicNum)
{
    H264PictureContext::ptr picture = nullptr;
    for (auto _picture : pictures)
    {
        if (_picture->referenceFlag & H264PictureContext::used_for_short_term_reference )
        {
            if (PicNum == _picture->PicNum)
            {
                picture = _picture;
                break;
            }
        }
    }
    assert(picture);
    return picture;
}

static H264PictureContext::ptr FindPictureByLongTermPicNum(H264PictureContext::cache pictures, uint64_t LongTermPicNum)
{
    H264PictureContext::ptr picture = nullptr;
    for (auto _picture : pictures)
    {
        if (_picture->referenceFlag & H264PictureContext::used_for_long_term_reference)
        {
            if (LongTermPicNum == _picture->LongTermPicNum)
            {
                picture = _picture;
                break;
            }
        }
    }
    assert(picture);
    return picture;
}

static void UnMarkUsedForShortTermReference(H264PictureContext::cache pictures, int32_t picNumX)
{
    for (auto _picture : pictures)
    {
        if (_picture->PicNum == picNumX && _picture->referenceFlag & H264PictureContext::used_for_short_term_reference)
        {
            if (_picture->field_pic_flag == 0)
            {
                _picture->referenceFlag = H264PictureContext::unused_for_reference;
            }
            else if (_picture->field_pic_flag == 1)
            {
                // _picture->referenceFlag = _picture->referenceFlag ^ H264PictureContext::used_for_short_term_reference;
                // H264PictureContext::ptr compPicture = FindComplementaryPicture(pictures, _picture);
                // if (compPicture)
                // {
                //     compPicture->referenceFlag = compPicture->referenceFlag ^ H264PictureContext::used_for_short_term_reference;
                // }
                // Hint : not support for now
                assert(false);
            }
            MPP_H264_SD_LOG("[RF] UnMarkUsedForShortTermReference, PicNum(%ld) FrameNum(%d)", _picture->PicNum, _picture->FrameNum);
            // Hint : 参考 FFmpeg 6.x 以及 openh264 , 此处应当只 umark 一个 short term picture, 按照 DPB 的顺序
            //        同时 ISO 中也存在 `a short-term reference picture` 而非 `all short-term refernce pictures`
            break;
        }
    }
}

static void UnMarkUsedForLongTermReference(H264PictureContext::cache pictures, uint32_t long_term_pic_num)
{
    for (auto _picture : pictures)
    {
        if (long_term_pic_num == _picture->LongTermPicNum)
        {
            if (_picture->field_pic_flag == 0)
            {
                _picture->referenceFlag = H264PictureContext::unused_for_reference;
                H264PictureContext::ptr compPicture = FindComplementaryPicture(pictures, _picture);
                if (compPicture)
                {
                    compPicture->referenceFlag = H264PictureContext::unused_for_reference;
                }
            }
            else if (_picture->field_pic_flag == 1)
            {
                // _picture->referenceFlag = _picture->referenceFlag ^ H264PictureContext::used_for_long_term_reference;
                // H264PictureContext::ptr compPicture = FindComplementaryPicture(pictures, _picture);
                // if (compPicture)
                // {
                //     compPicture->referenceFlag = compPicture->referenceFlag ^ H264PictureContext::used_for_long_term_reference;
                // }
                // Hint : not support for now
                assert(false);
            }
        }
    }
}

static void UnMarkUsedForReference(H264PictureContext::cache pictures, uint32_t long_term_pic_num)
{
    for (auto _picture : pictures)
    {
        if (long_term_pic_num == _picture->LongTermPicNum && _picture->referenceFlag & H264PictureContext::used_for_long_term_reference)
        {
            _picture->referenceFlag = H264PictureContext::unused_for_reference;
            H264PictureContext::ptr compPicture = FindComplementaryPicture(pictures, _picture);
            if (compPicture)
            {
                compPicture->referenceFlag = H264PictureContext::unused_for_reference;
            }
        }
    }
}

static void MarkShortTermReferenceToLongTermReference(H264PictureContext::cache pictures, int32_t picNumX, uint32_t long_term_frame_idx)
{
    for (auto _picture : pictures)
    {
        if (_picture->field_pic_flag == 0)
        {
            if (_picture->referenceFlag & H264PictureContext::used_for_short_term_reference && _picture->PicNum == picNumX)
            {
                H264PictureContext::ptr compPicture = FindComplementaryPicture(pictures, _picture);
                if (compPicture)
                {
                    compPicture->referenceFlag = H264PictureContext::used_for_long_term_reference;
                    compPicture->LongTermFrameIdx = long_term_frame_idx;
                }
                _picture->referenceFlag = H264PictureContext::used_for_long_term_reference;
                _picture->LongTermFrameIdx = long_term_frame_idx;
            }
        }
        else if  (_picture->field_pic_flag == 1)
        {
            // Hint : not support for now
            assert(false);
        }
    }
}

static int32_t PicOrderCnt(H264PictureContext::ptr picX) // (8-1)
{
    if (picX->field_pic_flag == 0)
    {
        return std::min(picX->TopFieldOrderCnt, picX->BottomFieldOrderCnt);
    }
    else if (picX->bottom_field_flag == 0 /* && picX->field_pic_flag == 1 */)
    {
        return picX->TopFieldOrderCnt;
    }
    else if (picX->bottom_field_flag == 1 /* && picX->field_pic_flag == 1 */)
    {
        return picX->BottomFieldOrderCnt;
    }
    else
    {
        assert(false);
        return 0;
    }
}

#if 0
static int32_t DiffPicOrderCnt(H264PictureContext::ptr picA, H264PictureContext::ptr picB) // (8-2)
{
    return PicOrderCnt(picA) - PicOrderCnt(picB);
}
#endif

/*************************************** 8.2.1 Decoding process for picture order count(Begin) ******************************************/
/**
 * @sa  
 *        1 - ISO 14496/10(2020) - 8.2.1 Decoding process for picture order count
 *        2 - ISO 14496/10(2020) - 7.4.1.2 Order of NAL units and association to coded pictures, access units, and video sequences
 * @note  picture order count (3.1.110)
 *        variable that is associated with each coded field and each field of a coded frame and has a value that is non-decreasing
 *        with increasing field position in output order relative to the first output field of the previous IDR picture in decoding
 *        order or relative to the first output field of the previous picture, in decoding order, that contains a memory management
 *        control operation that marks all reference pictures as "unused for reference"
 */
void H264SliceDecodingProcess::DecodingProcessForPictureOrderCount(H264NalSyntax::ptr nal, H264SpsSyntax::ptr sps, H264PpsSyntax::ptr pps, H264SliceHeaderSyntax::ptr slice, uint8_t nal_ref_idc, H264PictureContext::ptr picture)
{
    if (sps->pic_order_cnt_type > 2)
    {
        assert(false);
        return;
    }

    if (sps->pic_order_cnt_type == 0)
    {
        DecodeH264PictureOrderCountType0(_prevPicture, sps, pps, slice, nal_ref_idc, picture);
    }
    else if (sps->pic_order_cnt_type == 1)
    {
        DecodeH264PictureOrderCountType1(_prevPicture, nal, sps, slice, nal_ref_idc, picture);
    }
    else if (sps->pic_order_cnt_type == 2)
    {
        DecodeH264PictureOrderCountType2(_prevPicture, sps,  slice, nal_ref_idc, picture);
    }

    // Hint :
    //        When the current picture includes a
    //        memory_management_control_operation equal to 5, after the decoding of the current picture, tempPicOrderCnt is set
    //        equal to PicOrderCnt( CurrPic ), TopFieldOrderCnt of the current picture (if any) is set equal to
    //        TopFieldOrderCnt − tempPicOrderCnt, and BottomFieldOrderCnt of the current picture (if any) is set equal to
    //        BottomFieldOrderCnt − tempPicOrderCnt.
    _endTasks.push_back([picture]()
    {
        if (picture->has_memory_management_control_operation_5)
        {
            int32_t tempPicOrderCnt = PicOrderCnt(picture);
            picture->TopFieldOrderCnt = picture->TopFieldOrderCnt - tempPicOrderCnt;
            picture->BottomFieldOrderCnt = picture->BottomFieldOrderCnt - tempPicOrderCnt;
        }
    });
}

/**
 * @sa ISO 14496/10(2020) - 8.2.1.1 Decoding process for picture order count type 0
 */
void H264SliceDecodingProcess::DecodeH264PictureOrderCountType0(H264PictureContext::ptr prevPictrue, H264SpsSyntax::ptr sps, H264PpsSyntax::ptr pps, H264SliceHeaderSyntax::ptr slice, uint8_t nal_ref_idc, H264PictureContext::ptr picture)
{
    int32_t prevPicOrderCntMsb = 0;
    uint32_t prevPicOrderCntLsb = 0;
    int32_t PicOrderCntMsb = 0;
    // determine prevPicOrderCntMsb and prevPicOrderCntLsb
    {
        if (slice->slice_type == H264SliceType::MMP_H264_I_SLICE)
        {
            prevPicOrderCntMsb = 0;
            prevPicOrderCntLsb = 0;
        }
        else
        {
            if (prevPictrue->has_memory_management_control_operation_5)
            {
                if (!prevPictrue->bottom_field_flag)
                {
                    prevPicOrderCntMsb = 0;
                    prevPicOrderCntLsb  = prevPictrue->TopFieldOrderCnt;
                }
                else
                {
                    prevPicOrderCntMsb = 0;
                    prevPicOrderCntLsb = 0;
                }
            }
            else
            {
                prevPicOrderCntMsb = prevPictrue->prevPicOrderCntMsb;
                prevPicOrderCntLsb = prevPictrue->pic_order_cnt_lsb;
            }
        }
    }
    // determine PicOrderCntMsb (8-3)
    {
        uint32_t MaxPicOrderCntLsb = sps->context->MaxPicOrderCntLsb;
        if ((slice->pic_order_cnt_lsb < prevPicOrderCntLsb) &&
            ((prevPicOrderCntLsb - slice->pic_order_cnt_lsb) >= (MaxPicOrderCntLsb / 2))
        )
        {
            PicOrderCntMsb = (int32_t)(prevPicOrderCntMsb + MaxPicOrderCntLsb);
        }
        else if ((slice->pic_order_cnt_lsb > prevPicOrderCntLsb) && 
            ((slice->pic_order_cnt_lsb - prevPicOrderCntLsb) > (MaxPicOrderCntLsb / 2))
        )
        {
            PicOrderCntMsb = (int32_t)(prevPicOrderCntMsb - MaxPicOrderCntLsb);
        }
        else
        {
            PicOrderCntMsb = prevPicOrderCntMsb;
        }
    }
    // determine TopFieldOrderCnt and BottomFieldOrderCnt (8-4) and (8-5)
    {
        picture->TopFieldOrderCnt = PicOrderCntMsb + slice->pic_order_cnt_lsb;
        if (pps->bottom_field_pic_order_in_frame_present_flag && !slice->field_pic_flag)
        {
            picture->BottomFieldOrderCnt = picture->TopFieldOrderCnt + slice->delta_pic_order_cnt_bottom;
        }
        else
        {
            picture->BottomFieldOrderCnt = PicOrderCntMsb + slice->pic_order_cnt_lsb;
        }
    }
    // update context
    if (nal_ref_idc != 0)
    {
        picture->prevPicOrderCntMsb = PicOrderCntMsb;
    }
    else
    {
        picture->prevPicOrderCntMsb = prevPictrue->prevPicOrderCntMsb;
    }
}

/**
 * @sa ISO 14496/10(2020) - 8.2.1.2 Decoding process for picture order count type 1 
 */
void H264SliceDecodingProcess::DecodeH264PictureOrderCountType1(H264PictureContext::ptr prevPictrue, H264NalSyntax::ptr nal, H264SpsSyntax::ptr sps, H264SliceHeaderSyntax::ptr slice, uint8_t nal_ref_idc, H264PictureContext::ptr picture)
{
    uint32_t prevFrameNum = prevPictrue ? prevPictrue->FrameNum : 0;
    int32_t  prevFrameNumOffset = 0;
    int64_t  absFrameNum = 0;
    int64_t  picOrderCntCycleCnt = 0;
    int64_t  frameNumInPicOrderCntCycle = 0;
    int64_t  expectedPicOrderCnt;
    // determine prevFrameNumOffset
    {
        if (slice->slice_type != H264SliceType::MMP_H264_I_SLICE)
        {
            if (prevPictrue->has_memory_management_control_operation_5)
            {
                prevFrameNumOffset = 0;
            }
            else
            {
                prevFrameNumOffset = (int32_t)(prevPictrue->FrameNumOffset);
            }
        }
    }
    // determine FrameNumOffset (8-6)
    {
        if (nal->nal_unit_type == H264NaluType::MMP_H264_NALU_TYPE_IDR) // (7-0)
        {
            picture->FrameNumOffset = 0;
        }
        else if (prevFrameNum > slice->frame_num)
        {
            uint32_t MaxFrameNum = sps->context->MaxFrameNum;
            picture->FrameNumOffset = prevFrameNumOffset + MaxFrameNum;
        }
        else
        {
            picture->FrameNumOffset = prevFrameNumOffset;
        }
    }
    // determine absFrameNum (8-7)
    {
        if (sps->num_ref_frames_in_pic_order_cnt_cycle != 0)
        {
            absFrameNum = picture->FrameNumOffset + slice->frame_num;
        }
        else
        {
            absFrameNum = 0;
        }
        if (nal_ref_idc == 0 && absFrameNum > 0)
        {
            absFrameNum = absFrameNum - 1;
        }
    }
    // determine picOrderCntCycleCnt and frameNumInPicOrderCntCycle (8-8)
    {
        if (absFrameNum > 0)
        {
            picOrderCntCycleCnt = (absFrameNum - 1) / sps->num_ref_frames_in_pic_order_cnt_cycle;
            frameNumInPicOrderCntCycle = (absFrameNum - 1) % sps->num_ref_frames_in_pic_order_cnt_cycle;
        }
    }
    // determine expectedPicOrderCnt (8-9)
    {
        if (absFrameNum > 0)
        {
            int64_t ExpectedDeltaPerPicOrderCntCycle = sps->context->ExpectedDeltaPerPicOrderCntCycle; 
            expectedPicOrderCnt = picOrderCntCycleCnt * ExpectedDeltaPerPicOrderCntCycle;
            for (int64_t i=0; i<=frameNumInPicOrderCntCycle; i++)
            {
                expectedPicOrderCnt = expectedPicOrderCnt + sps->offset_for_ref_frame[i];
            }
        }
        else
        {
            expectedPicOrderCnt = 0;
        }
        if (nal_ref_idc == 0)
        {
            expectedPicOrderCnt = expectedPicOrderCnt + sps->offset_for_non_ref_pic;
        }
    }
    // determine TopFieldOrderCnt and BottomFieldOrderCnt (8-10)
    {
        if (!slice->field_pic_flag)
        {
            picture->TopFieldOrderCnt = (int32_t)(expectedPicOrderCnt + slice->delta_pic_order_cnt[0]);
            picture->BottomFieldOrderCnt = picture->TopFieldOrderCnt + sps->offset_for_top_to_bottom_field + slice->delta_pic_order_cnt[1];
        }
        else if (!slice->bottom_field_flag)
        {
            picture->TopFieldOrderCnt = (int32_t)(expectedPicOrderCnt + slice->delta_pic_order_cnt[0]);
        }
        else
        {
            picture->BottomFieldOrderCnt = (int32_t)(expectedPicOrderCnt + sps->offset_for_top_to_bottom_field + slice->delta_pic_order_cnt[0]);
        }
    }
}

/**
 * @sa ISO 14496/10(2020) - 8.2.1.3 Decoding process for picture order count type 2
 */
void H264SliceDecodingProcess::DecodeH264PictureOrderCountType2(H264PictureContext::ptr prevPictrue, H264SpsSyntax::ptr sps, H264SliceHeaderSyntax::ptr slice, uint8_t nal_ref_idc, H264PictureContext::ptr picture)
{
    int64_t FrameNumOffset = 0;
    int64_t tempPicOrderCnt = 0;
    uint64_t prevFrameNumOffset = 0;
    // determine prevFrameNumOffset
    {
        if (slice->slice_type != H264SliceType::MMP_H264_I_SLICE)
        {
            if (prevPictrue->has_memory_management_control_operation_5)
            {
                prevFrameNumOffset = 0;
            }
            else
            {
                prevFrameNumOffset = prevPictrue->FrameNumOffset;
            }
        }
    }
    // determine FrameNumOffset (8-11)
    {
        if (slice->slice_type == H264SliceType::MMP_H264_I_SLICE)
        {
            FrameNumOffset = 0;
        }
        else if (prevFrameNumOffset > slice->frame_num)
        {
            uint32_t MaxFrameNum = sps->context->MaxFrameNum;
            FrameNumOffset = prevFrameNumOffset + MaxFrameNum;
        }
        else
        {
            FrameNumOffset = prevFrameNumOffset;
        }
        picture->FrameNumOffset = FrameNumOffset;
    }
    // determine tempPicOrderCnt (8-12)
    {
        if (slice->slice_type == H264SliceType::MMP_H264_I_SLICE)
        {
            tempPicOrderCnt = 0;
        }
        else if (nal_ref_idc == 0)
        {
            tempPicOrderCnt = 2 * (FrameNumOffset + slice->frame_num) - 1;
        }
        else
        {
            tempPicOrderCnt = 2 * (FrameNumOffset + slice->frame_num);
        }
    }
    // determine TopFieldOrderCnt and BottomFieldOrderCnt (8-13)
    {
        if (!slice->field_pic_flag)
        {
            picture->TopFieldOrderCnt = (int32_t)tempPicOrderCnt;
            picture->BottomFieldOrderCnt = (int32_t)tempPicOrderCnt;
        }
        else if (slice->bottom_field_flag)
        {
            picture->BottomFieldOrderCnt = (int32_t)tempPicOrderCnt;
        }
        else
        {
            picture->TopFieldOrderCnt = (int32_t)tempPicOrderCnt;
        }
    }
}

/*************************************** 8.2.1 Decoding process for picture order count(End) ******************************************/

/*************************************** 8.2.4 Decoding process for reference picture lists construction(Begin) ******************************************/

/**
 * @sa  ISO 14496/10(2020) - 8.2.4 Decoding process for reference picture lists construction
 */
void H264SliceDecodingProcess::DecodingProcessForReferencePictureListsConstruction(H264SliceHeaderSyntax::ptr slice, H264SpsSyntax::ptr sps, H264PictureContext::cache pictures, H264PictureContext::ptr picture)
{
    DecodingProcessForPictureNumbers(slice, sps, pictures, picture);
    InitializationProcessForReferencePictureLists(slice, pictures, picture);
    if (slice->rplm->ref_pic_list_modification_flag_l0 || 
        (slice->rplm->ref_pic_list_modification_flag_l1 && slice->slice_type == H264SliceType::MMP_H264_B_SLICE)
    )
    {
        ModificationProcessForReferencePictureLists(slice, sps, pictures, picture);
    }
}

/**
 * @sa  ISO 14496/10(2020) - 8.2.4.1 Decoding process for picture numbers
 */
void H264SliceDecodingProcess::DecodingProcessForPictureNumbers(H264SliceHeaderSyntax::ptr slice, H264SpsSyntax::ptr sps, H264PictureContext::cache pictures, H264PictureContext::ptr picture)
{
    // determine FrameNumWrap (8-27)
    {
        uint32_t MaxFrameNum = sps->context->MaxFrameNum;
        for (auto _picture : pictures)
        {
            if (_picture->referenceFlag & H264PictureContext::used_for_short_term_reference)
            {
                if (_picture->FrameNum > slice->frame_num)
                {
                    _picture->FrameNumWrap = (int32_t)_picture->FrameNum - (int32_t)MaxFrameNum;
                }
                else
                {
                    _picture->FrameNumWrap = _picture->FrameNum;
                }
            }
        }
    }
    // determine PicNum and LongTermPicNum
    {
        for (auto _picture : pictures)
        {
            if (_picture->field_pic_flag == 0)
            {
                if (_picture->referenceFlag & H264PictureContext::used_for_short_term_reference)
                {
                    _picture->PicNum = _picture->FrameNumWrap; // (8-28)
                }
                if (_picture->referenceFlag & H264PictureContext::used_for_long_term_reference)
                {
                    _picture->LongTermPicNum = (uint32_t)_picture->LongTermFrameIdx; // (8-29)
                }
            }
            else if (_picture->field_pic_flag == 1)
            {
                // TOCHECK : top field 一定出现在 bottom field 之前吗, 如何确定 same parity 和 oppsite partity
                // WORKAROUND : 
                //              1) 是不是直接不支持场编码问题就都解决了, 当前 h264 场编码已经应用得比较少了;
                //                 支持场编码一方面逻辑异常复杂,另一方面也不好进行测试验证
                //              2) 确认一下 FFmpeg 6.x (FFmpeg/libavcodec/h264_refs.c) 这部分的代码逻辑 (,但是看起来不是很好确认 ...) 
                // if (_picture->referenceFlag & H264PictureContext::used_for_short_term_reference)
                // {
                //     if (_picture->bottom_field_flag)
                //     {
                //         _picture->PicNum = 2 * _picture->FrameNumWrap + 1; // (8-30)
                //     }
                //     else
                //     {
                //         _picture->PicNum = 2 * _picture->FrameNumWrap; // (8-31)
                //     }
                // }
                // if (_picture->referenceFlag & H264PictureContext::used_for_long_term_reference)
                // {
                //     if (_picture->bottom_field_flag)
                //     {
                //         _picture->LongTermPicNum = 2 * _picture->LongTermFrameIdx + 1;
                //     }
                //     else
                //     {
                //         _picture->LongTermPicNum = 2 * picture->LongTermFrameIdx;
                //     }
                // }
                // Hint : not support for now
                assert(false);
            }
        }
    }
}

/**
 * @sa  ISO 14496/10(2020) - 8.2.4.2 Initialization process for reference picture lists
 */
void H264SliceDecodingProcess::InitializationProcessForReferencePictureLists(H264SliceHeaderSyntax::ptr slice, H264PictureContext::cache pictures, H264PictureContext::ptr picture)
{
    {
        _RefPicList0.clear();
        _RefPicList1.clear();
    }
    // 8.2.4.2.1 Initialization process for the reference picture list for P and SP slices in frames
    if ((slice->slice_type == H264SliceType::MMP_H264_P_SLICE || slice->slice_type == H264SliceType::MMP_H264_SP_SLICE) && slice->field_pic_flag == 0)
    {
        std::vector<H264PictureContext::ptr> shortTermRefPicList; // the highest PicNum value in descending order
        std::vector<H264PictureContext::ptr> longTermRefList;     // the lowest LongTermPicNum value in ascending order
        for (auto picture : pictures)
        {
            if (picture->referenceFlag & H264PictureContext::used_for_short_term_reference)
            {
                shortTermRefPicList.push_back(picture);
            }
            if (picture->referenceFlag & H264PictureContext::used_for_long_term_reference)
            {
                longTermRefList.push_back(picture);
            }
        }
        std::sort(shortTermRefPicList.begin(), shortTermRefPicList.end(), [](const H264PictureContext::ptr& left, const H264PictureContext::ptr& right) -> bool
        {
            return left->PicNum > right->PicNum;
        });
        std::sort(longTermRefList.begin(), longTermRefList.end(), [](const H264PictureContext::ptr& left, const H264PictureContext::ptr& right) -> bool
        {
            return left->LongTermPicNum < right->LongTermPicNum;
        });
        MPP_H264_SD_LOG("-- shortTermRefPicList(%ld) longTermRefList(%ld)", shortTermRefPicList.size(), longTermRefList.size());
        for (auto& shortTermPicture : shortTermRefPicList)
        {
            _RefPicList0.push_back(shortTermPicture);
        }
        for (auto& longTermPicture : longTermRefList)
        {
            _RefPicList0.push_back(longTermPicture);
        }
    }
    // 8.2.4.2.2 Initialization process for the reference picture list for P and SP slices in fields
    else if ((slice->slice_type == H264SliceType::MMP_H264_P_SLICE || slice->slice_type == H264SliceType::MMP_H264_SP_SLICE) && slice->field_pic_flag == 1)
    {
        // Hint : not support field for now
        assert(false);
    }
    // 8.2.4.2.3 Initialization process for reference picture lists for B slices in frames
    else if ((slice->slice_type == H264SliceType::MMP_H264_B_SLICE) && slice->field_pic_flag == 0)
    {
        int32_t curPoc = PicOrderCnt(picture);
        {
            std::deque<H264PictureContext::ptr>  RefPicList01;    // short term : PicOrderCnt( entryShortTerm ) less than PicOrderCnt( CurrPic ) in descending order
            std::vector<H264PictureContext::ptr> RefPicList02;    // short term :  others in ascending order
            std::vector<H264PictureContext::ptr> RefPicList03;    // long term  :  the lowest LongTermPicNum value in ascending order
            {
                H264PictureContext::ptr minPicture = nullptr;
                for (auto __picture : _pictures)
                {
                    if (__picture->referenceFlag & H264PictureContext::used_for_short_term_reference)
                    {
                        if (PicOrderCnt(__picture) < curPoc)
                        {
                            RefPicList01.push_back(__picture);
                        }
                    }
                }
                std::sort(RefPicList01.begin(), RefPicList01.end(), [](const H264PictureContext::ptr& left, const H264PictureContext::ptr& right)->bool
                {
                    return PicOrderCnt(left) > PicOrderCnt(right);
                });
            }
            {
                for (auto __picture : _pictures)
                {
                    if (__picture->referenceFlag & H264PictureContext::used_for_short_term_reference)
                    {
                        if (PicOrderCnt(__picture) > curPoc)
                        {
                            RefPicList02.push_back(__picture);
                        }
                    }
                }
                std::sort(RefPicList02.begin(), RefPicList02.end(), [](const H264PictureContext::ptr& left, const H264PictureContext::ptr& right)->bool
                {
                    return PicOrderCnt(left) < PicOrderCnt(right);
                });
            }
            {
                for (auto picture : pictures)
                {
                    if (picture->referenceFlag & H264PictureContext::used_for_long_term_reference)
                    {
                        RefPicList03.push_back(picture);
                    }
                }
                std::sort(RefPicList03.begin(), RefPicList03.end(), [](const H264PictureContext::ptr& left, const H264PictureContext::ptr& right) -> bool
                {
                    return left->LongTermPicNum < right->LongTermPicNum;
                });
            }
            MPP_H264_SD_LOG("-- RefPicList01(%ld) RefPicList02(%ld) RefPicList03(%ld)", RefPicList01.size(), RefPicList02.size(), RefPicList03.size());
            for (const auto& RefPic : RefPicList01)
            {
                _RefPicList0.push_back(RefPic);
            }
            for (const auto& RefPic : RefPicList02)
            {
                _RefPicList0.push_back(RefPic);
            }
            for (const auto& RefPic : RefPicList03)
            {
                _RefPicList0.push_back(RefPic);
            }
        }
        {
            std::vector<H264PictureContext::ptr> RefPicList11;    // short term : PicOrderCnt( entryShortTerm ) greater than PicOrderCnt( CurrPic ) in descending order
            std::vector<H264PictureContext::ptr> RefPicList12;    // short term :  others in ascending order
            std::vector<H264PictureContext::ptr> RefPicList13;    // long term  :  the lowest LongTermPicNum value in ascending order
            {
                for (auto __picture : _pictures)
                {
                    if (__picture->referenceFlag & H264PictureContext::used_for_short_term_reference)
                    {
                        int32_t picPoc = PicOrderCnt(__picture);
                        if (picPoc > curPoc)
                        {
                            RefPicList11.push_back(__picture);
                        }
                    }
                }
                std::sort(RefPicList11.begin(), RefPicList11.end(), [](const H264PictureContext::ptr& left, const H264PictureContext::ptr& right)->bool
                {
                    return PicOrderCnt(left) < PicOrderCnt(right);
                });
            }
            {
                for (auto __picture : _pictures)
                {
                    if (__picture->referenceFlag & H264PictureContext::used_for_short_term_reference)
                    {
                        int32_t picPoc = PicOrderCnt(__picture);
                        if (picPoc < curPoc)
                        {
                            RefPicList12.push_back(__picture);
                        }
                    }
                }
                std::sort(RefPicList12.begin(), RefPicList12.end(), [](const H264PictureContext::ptr& left, const H264PictureContext::ptr& right)->bool
                {
                    return PicOrderCnt(left) > PicOrderCnt(right);
                });
            }
            {
                for (auto picture : pictures)
                {
                    if (picture->referenceFlag & H264PictureContext::used_for_long_term_reference)
                    {
                        RefPicList13.push_back(picture);
                    }
                }
                std::sort(RefPicList13.begin(), RefPicList13.end(), [](const H264PictureContext::ptr& left, const H264PictureContext::ptr& right) -> bool
                {
                    return left->LongTermPicNum > right->LongTermPicNum;
                });
            }
            MPP_H264_SD_LOG("-- RefPicList11(%ld) RefPicList12(%ld) RefPicList13(%ld)", RefPicList11.size(), RefPicList12.size(), RefPicList13.size());
            for (const auto& RefPic : RefPicList11)
            {
                _RefPicList1.push_back(RefPic);
            }
            for (const auto& RefPic : RefPicList12)
            {
                _RefPicList1.push_back(RefPic);
            }
            for (const auto& RefPic : RefPicList13)
            {
                _RefPicList1.push_back(RefPic);
            }
        }
    }
    else if ((slice->slice_type == H264SliceType::MMP_H264_B_SLICE) && slice->field_pic_flag == 1)
    {
        // Hint : not support for now
        assert(false);
    }
    {
        _RefPicList0.resize(slice->num_ref_idx_l0_active_minus1 + 1);
        _RefPicList1.resize(slice->num_ref_idx_l1_active_minus1 + 1);
    }
#ifdef ENABLE_MMP_SD_DEBUG
    static auto refTypeToStr = [](uint64_t referenceFlag) -> std::string
    {
        if (referenceFlag & H264PictureContext::used_for_short_term_reference)
        {
            return "ST";
        }
        else if (referenceFlag & H264PictureContext::used_for_long_term_reference)
        {
            return "LT";
        }
        else
        {
            assert(false);
            return "?";
        }
    };
    {
        std::stringstream ss;
        ss << std::endl << "[IPRC] RefPicList0[" << _RefPicList0.size() << "]" << std::endl;
        for (size_t i=0; i<_RefPicList0.size() && _RefPicList0[i]; i++)
        {
            ss << "-- (" << i << ") Type(" << refTypeToStr(_RefPicList0[i]->referenceFlag) <<  ") "
               << "FrameNum(" << _RefPicList0[i]->FrameNum << ")";
            if (_RefPicList0[i]->referenceFlag & H264PictureContext::used_for_short_term_reference)
            {
                ss << " PicOrderCnt(" << PicOrderCnt(_RefPicList0[i]) << ") PicNum(" << _RefPicList0[i]->PicNum << ")";
            }
            else if (_RefPicList0[i]->referenceFlag & H264PictureContext::used_for_long_term_reference)
            {
                ss << " LongTermPicNum(" << _RefPicList0[i]->LongTermPicNum << ")";
            }
            if (i + 1 != _RefPicList0.size())
            {
                ss << std::endl;
            }
        }
        std::string log = ss.str();
        MPP_H264_SD_LOG("%s", log.c_str());
    }
    {
        std::stringstream ss;
        ss << std::endl << "[IPRC] RefPicList1[" << _RefPicList1.size() << "]" << std::endl;
        for (size_t i=0; i<_RefPicList1.size() && _RefPicList1[i]; i++)
        {
            ss << "-- (" << i << ") Type(" << refTypeToStr(_RefPicList1[i]->referenceFlag) <<  ") "
               << "FrameNum(" << _RefPicList1[i]->FrameNum << ")";
            if (_RefPicList0[i]->referenceFlag & H264PictureContext::used_for_short_term_reference)
            {
                ss << " PicOrderCnt(" << PicOrderCnt(_RefPicList0[i]) << ") PicNum(" << _RefPicList0[i]->PicNum << ")";
            }
            else if (_RefPicList0[i]->referenceFlag & H264PictureContext::used_for_long_term_reference)
            {
                ss << " LongTermPicNum(" << _RefPicList0[i]->LongTermPicNum << ")";
            }
            if (i + 1 != _RefPicList1.size())
            {
                ss << std::endl;
            }
        }
        std::string log = ss.str();
        MPP_H264_SD_LOG("%s", log.c_str());
    }
#endif /* ENABLE_MMP_SD_DEBUG */
}

/**
 * @sa  ISO 14496/10(2020) - 8.2.4.3 Modification process for reference picture lists
 */
void H264SliceDecodingProcess::ModificationProcessForReferencePictureLists(H264SliceHeaderSyntax::ptr slice, H264SpsSyntax::ptr sps, H264PictureContext::cache pictures, H264PictureContext::ptr picture)
{
    uint64_t MaxPicNum = 0;
    uint64_t CurrPicNum = 0;
    int64_t picNumLX = 0;
    size_t index = 0;
    // determine CurrPicNum
    // The variable CurrPicNum is derived as follows:
    // - If field_pic_flag is equal to 0, CurrPicNum is set equal to frame_num.
    // - Otherwise (field_pic_flag is equal to 1), CurrPicNum is set equal to 2 * frame_num + 1.
    {
        if (slice->field_pic_flag == 0)
        {
            CurrPicNum = slice->frame_num;
        }
        else if (slice->field_pic_flag == 1)
        {
            // CurrPicNum = 2 * slice->frame_num + 1;
            // Hint : not support for now
            assert(false);
        }
    }
    // determine MaxPicNum
    // The variable MaxPicNum is derived as follows:
    // - If field_pic_flag is equal to 0, MaxPicNum is set equal to MaxFrameNum.
    // - Otherwise (field_pic_flag is equal to 1), MaxPicNum is set equal to 2*MaxFrameNum.
    {
        uint32_t MaxFrameNum = sps->context->MaxFrameNum;
        if (slice->field_pic_flag == 0)
        {
            MaxPicNum = MaxFrameNum;
        }
        else if (slice->field_pic_flag == 1)
        {
            // MaxPicNum = 2 * MaxFrameNum;
            // Hint : not support for now
            assert(false);
        }
    }

    auto modificationProcessForReferencePictureLists = [&](uint32_t& refIdxLX, std::vector<H264PictureContext::ptr>& RefPicListX, uint32_t num_ref_idx_lX_active_minus1)
    {
        uint64_t picNumLXPred = 0;
        bool initPicNumLXPred = false;
        for (const auto& modification_of_pic_nums_idc : slice->rplm->modification_of_pic_nums_idcs)
        {
            MPP_H264_SD_LOG("[MRPL] modification_of_pic_nums_idc(%d)", modification_of_pic_nums_idc);
            // 8.2.4.3.1 Modification process of reference picture lists for short-term reference pictures
            if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1)
            {
                // Hint :
                //   When the process specified in this subclause
                // is invoked the first time for a slice (that is, for the first occurrence of modification_of_pic_nums_idc equal to 0 or 1 in the
                // ref_pic_list_modification( ) syntax), picNumL0Pred and picNumL1Pred are initially set equal to CurrPicNum.
                if (initPicNumLXPred == false)
                {
                    picNumLXPred = CurrPicNum;
                    initPicNumLXPred = true;
                }
                uint64_t picNumLXNoWrap = 0;
                uint32_t abs_diff_pic_num_minus1 = slice->rplm->modification_of_pic_nums_idcs_datas[index++].abs_diff_pic_num_minus1;
                // determine picNumLXNoWrap
                {
                    if (modification_of_pic_nums_idc == 0) // (8-34)
                    {
                        if ((int64_t)picNumLXPred - (abs_diff_pic_num_minus1 + 1) < 0)
                        {
                            picNumLXNoWrap = picNumLXPred - (abs_diff_pic_num_minus1 + 1) + MaxPicNum;
                        }
                        else
                        {
                            picNumLXNoWrap = picNumLXPred - (abs_diff_pic_num_minus1 + 1);
                        }
                    }
                    else if (modification_of_pic_nums_idc == 1) // (8-35)
                    {
                        if (picNumLXPred + (abs_diff_pic_num_minus1 + 1) >= MaxPicNum)
                        {
                            picNumLXNoWrap = picNumLXPred + (abs_diff_pic_num_minus1 + 1) - MaxPicNum;
                        }
                        else
                        {
                            picNumLXNoWrap = picNumLXPred + (abs_diff_pic_num_minus1 + 1);
                        }
                    }
                }
                // Hint : After each assignment of picNumLXNoWrap, the value of picNumLXNoWrap is assigned to picNumLXPred.
                picNumLXPred = picNumLXNoWrap;
                // determine picNumLX (8-36)
                {
                    if (picNumLXNoWrap > CurrPicNum)
                    {
                        picNumLX = picNumLXNoWrap - MaxPicNum;
                    }
                    else
                    {
                        picNumLX = picNumLXNoWrap;
                    }
                }
                // (8-37)
                {
                    auto PicNumF = [MaxPicNum](H264PictureContext::ptr picture) -> int64_t
                    {
                        // Hint :
                        // The function PicNumF( RefPicListX[ cIdx ] ) is derived as follows:
                        // - If the picture RefPicListX[ cIdx ] is marked as "used for short-term reference", PicNumF( RefPicListX[ cIdx ] ) is
                        //   the PicNum of the picture RefPicListX[ cIdx ].
                        // - Otherwise (the picture RefPicListX[ cIdx ] is not marked as "used for short-term reference"),
                        //   PicNumF( RefPicListX[ cIdx ] ) is equal to MaxPicNum.
                        if (picture && picture->referenceFlag & H264PictureContext::used_for_short_term_reference)
                        {
                            return picture->PicNum;
                        }
                        else
                        {
                            return (int64_t)MaxPicNum;
                        }
                    };
                    // Hint : the length of the list RefPicListX is temporarily made one element longer than the length needed for the final list
                    RefPicListX.resize((num_ref_idx_lX_active_minus1+1)+1);
                    for (uint32_t cIdx = num_ref_idx_lX_active_minus1+1; cIdx > refIdxLX; cIdx--)
                    {
                        RefPicListX[cIdx] = RefPicListX[cIdx-1];
                    }
                    MPP_H264_SD_LOG("[MRPL] refIdxLX(%d) abs_diff_pic_num_minus1(%d) picNumLX(%ld)", refIdxLX, abs_diff_pic_num_minus1, picNumLX);
                    RefPicListX[refIdxLX++] = FindPictureByPicNum(pictures, picNumLX); // short-term reference picture with PicNum equal to picNumLX
                    uint32_t nIdx = refIdxLX;
                    for (uint32_t cIdx = refIdxLX; cIdx <= num_ref_idx_lX_active_minus1+1; cIdx++)
                    {
                        if (PicNumF(RefPicListX[cIdx]) != picNumLX)
                        {
                            RefPicListX[nIdx++] = RefPicListX[cIdx];
                        }
                    }
                    for (; nIdx <= num_ref_idx_lX_active_minus1+1; nIdx++)
                    {
                        RefPicListX[nIdx++] = nullptr;
                    }
                    // Hint :  After the execution of this procedure, only elements 0 through num_ref_idx_lX_active_minus1 of the list need to be retained.
                    RefPicListX.resize(num_ref_idx_lX_active_minus1+1);
                }
            }
            // 8.2.4.3.2 Modification process of reference picture lists for long-term reference pictures
            else if (modification_of_pic_nums_idc == 2)
            {
                uint32_t long_term_pic_num = slice->rplm->modification_of_pic_nums_idcs_datas[index++].long_term_pic_num;
                auto LongTermPicNumF = [pictures, MaxPicNum, picture](uint32_t cIdx) -> uint32_t
                {
                    for (auto _picture : pictures)
                    {
                        if (_picture->LongTermFrameIdx == cIdx)
                        {
                            if (_picture->referenceFlag & H264PictureContext::used_for_long_term_reference)
                            {
                                return cIdx;
                            }
                        }
                    }
                    return (uint32_t)(2 * (picture->MaxLongTermFrameIdx + 1));
                };
                // Hint : the length of the list RefPicListX is temporarily made one element longer than the length needed for the final list
                RefPicListX.resize((num_ref_idx_lX_active_minus1+1)+1);
                for (uint32_t cIdx = num_ref_idx_lX_active_minus1 + 1; cIdx > refIdxLX; cIdx--)
                {
                    RefPicListX[cIdx] = RefPicListX[cIdx - 1];
                }
                MPP_H264_SD_LOG("[MRPL] refIdxLX(%d) long_term_pic_num(%d) LongTermPicNum(%d)", refIdxLX, long_term_pic_num, long_term_pic_num);
                RefPicListX[refIdxLX++] = FindPictureByLongTermPicNum(pictures, long_term_pic_num);
                uint32_t nIdx = refIdxLX;
                for (uint32_t cIdx = refIdxLX; cIdx <= num_ref_idx_lX_active_minus1 + 1; cIdx++)
                {
                    if (LongTermPicNumF(RefPicListX[cIdx]->LongTermPicNum) != long_term_pic_num)
                    {
                        RefPicListX[nIdx++] = RefPicListX[cIdx];
                    }
                }
                for (; nIdx <= num_ref_idx_lX_active_minus1+1; nIdx++)
                {
                    RefPicListX[nIdx++] = nullptr;
                }
                // Hint :  After the execution of this procedure, only elements 0 through num_ref_idx_lX_active_minus1 of the list need to be retained.
                RefPicListX.resize(num_ref_idx_lX_active_minus1+1);
            }
            else if (modification_of_pic_nums_idc == 3)
            {
                break;
            }
        }
    };

    if (slice->rplm->ref_pic_list_modification_flag_l0)
    {
        uint32_t refIdxL0 = 0;
        modificationProcessForReferencePictureLists(refIdxL0, _RefPicList0, slice->num_ref_idx_l0_active_minus1);
    }
    if (slice->rplm->ref_pic_list_modification_flag_l1 && slice->slice_type == H264SliceType::MMP_H264_B_SLICE)
    {
        uint32_t refIdxL1 = 0;
        modificationProcessForReferencePictureLists(refIdxL1, _RefPicList1, slice->num_ref_idx_l1_active_minus1);
    }
#ifdef ENABLE_MMP_SD_DEBUG
    static auto refTypeToStr = [](uint64_t referenceFlag) -> std::string
    {
        if (referenceFlag & H264PictureContext::used_for_short_term_reference)
        {
            return "ST";
        }
        else if (referenceFlag & H264PictureContext::used_for_long_term_reference)
        {
            return "LT";
        }
        else
        {
            return "?";
        }
    };
    {
        std::stringstream ss;
        ss << std::endl << "[MRPL] RefPicList0[" << _RefPicList0.size() << "]" << std::endl;
        for (size_t i=0; i<_RefPicList0.size() && _RefPicList0[i]; i++)
        {
            ss << "-- (" << i << ") Type(" << refTypeToStr(_RefPicList0[i]->referenceFlag) <<  ") "
               << "FrameNum(" << _RefPicList0[i]->FrameNum << ")";
            if (_RefPicList0[i]->referenceFlag & H264PictureContext::used_for_short_term_reference)
            {
                ss << " PicOrderCnt(" << PicOrderCnt(_RefPicList0[i]) << ")";
            }
            else if (_RefPicList0[i]->referenceFlag & H264PictureContext::used_for_long_term_reference)
            {
                ss << " LongTermPicNum(" << _RefPicList0[i]->LongTermPicNum << ")";
            }
            if (i + 1 != _RefPicList0.size())
            {
                ss << std::endl;
            }
        }
        std::string log = ss.str();
        MPP_H264_SD_LOG("%s", log.c_str());
    }
    {
        std::stringstream ss;
        ss << std::endl << "[MRPL] RefPicList1[" << _RefPicList1.size() << "]" << std::endl;
        for (size_t i=0; i<_RefPicList1.size() && _RefPicList1[i]; i++)
        {
            ss << "-- (" << i << ") Type(" << refTypeToStr(_RefPicList1[i]->referenceFlag) <<  ") "
               << "FrameNum(" << _RefPicList1[i]->FrameNum << ")";
            if (_RefPicList0[i]->referenceFlag & H264PictureContext::used_for_short_term_reference)
            {
                ss << " PicOrderCnt(" << PicOrderCnt(_RefPicList0[i]) << ")";
            }
            else if (_RefPicList0[i]->referenceFlag & H264PictureContext::used_for_long_term_reference)
            {
                ss << " LongTermPicNum(" << _RefPicList0[i]->LongTermPicNum << ")";
            }
            if (i + 1 != _RefPicList1.size())
            {
                ss << std::endl;
            }
        }
        std::string log = ss.str();
        MPP_H264_SD_LOG("%s", log.c_str());
    }
#endif /* ENABLE_MMP_SD_DEBUG */
}

/*************************************** 8.2.4 Decoding process for reference picture lists construction(End) ******************************************/

/*************************************** 8.2.5 Decoded reference picture marking process(Begin) ******************************************/

/**
 * @sa ISO 14496/10(2020) - 8.2.5 Decoded reference picture marking process
 */
void H264SliceDecodingProcess::DecodeReferencePictureMarkingProcess(H264NalSyntax::ptr nal, H264SliceHeaderSyntax::ptr slice, H264SpsSyntax::ptr sps, H264PictureContext::cache pictures, H264PictureContext::ptr picture, uint8_t nal_ref_idc)
{
    // Hint : A decoded picture with nal_ref_idc not equal to 0, referred to as a reference picture, is marked as "used for short-term reference" or "used for long-term reference".
    //        - decoded reference frame : both of its fields are marked the same as the frame
    //        - complementary reference field pair : the pair is marked the same as both of its fields
    if (nal_ref_idc == 0)
    {
        return;
    }
    SequenceOfOperationsForDecodedReferencePictureMarkingProcess(nal, slice, sps, pictures, picture);
}

/**
 * @sa ISO 14496/10(2020) - 8.2.5.1 Sequence of operations for decoded reference picture marking process 
 */
void H264SliceDecodingProcess::SequenceOfOperationsForDecodedReferencePictureMarkingProcess(H264NalSyntax::ptr nal, H264SliceHeaderSyntax::ptr slice, H264SpsSyntax::ptr sps, H264PictureContext::cache pictures, H264PictureContext::ptr picture)
{
    if (nal->nal_unit_type == H264NaluType::MMP_H264_NALU_TYPE_IDR)
    {
        if (slice->drpm->long_term_reference_flag == 0)
        {
            picture->referenceFlag = H264PictureContext::used_for_short_term_reference;
            picture->MaxLongTermFrameIdx = no_long_term_frame_indices;
        }
        else if (slice->drpm->long_term_reference_flag == 1)
        {
            picture->referenceFlag = H264PictureContext::used_for_long_term_reference;
            picture->LongTermFrameIdx = 0;
            picture->MaxLongTermFrameIdx = 0;
        }
    }
    else
    {
        // See also : ISO 14496/10(2020) - Table 7-8 – Interpretation of adaptive_ref_pic_marking_mode_flag
        if (slice->drpm->adaptive_ref_pic_marking_mode_flag == 0) /* Sliding window reference picture marking mode */
        {
            SlidingWindowDecodedReferencePictureMarkingProcess(slice, sps, pictures, picture);
        }
        else /* Adaptive reference picture marking mode */
        {
            AdaptiveMemoryControlDecodedReferencePicutreMarkingPorcess(slice, pictures, picture);
        }
    }
    if (slice->slice_type != H264SliceType::MMP_H264_I_SLICE && !(picture->referenceFlag & H264PictureContext::used_for_long_term_reference))
    {
        picture->referenceFlag = H264PictureContext::used_for_short_term_reference;
    }
}

/**
 * @sa ISO 14496/10(2020) - 8.2.5.2 Decoding process for gaps in frame_num
 */
void H264SliceDecodingProcess::DecodingProcessForGapsInFrameNum(H264SliceHeaderSyntax::ptr slice, H264SpsSyntax::ptr sps, H264PictureContext::ptr picture, uint64_t PrevRefFrameNum)
{

    uint32_t MaxFrameNum = sps->context->MaxFrameNum;
    if (!(slice->frame_num != PrevRefFrameNum && slice->frame_num != (PrevRefFrameNum + 1) % MaxFrameNum))
    {
        return;
    }
    assert(false);
    // Hint : no support for now
}

/**
 * @sa ISO 14496/10(2020) - 8.2.5.3 Sliding window decoded reference picture marking process
 */
void H264SliceDecodingProcess::SlidingWindowDecodedReferencePictureMarkingProcess(H264SliceHeaderSyntax::ptr slice, H264SpsSyntax::ptr sps, H264PictureContext::cache pictures, H264PictureContext::ptr picture)
{
    if (PictureIsSecondField(picture))
    {
        H264PictureContext::ptr compPicture = FindComplementaryPicture(pictures, picture);
        if (!compPicture)
        {
            return;
        }
        if (compPicture->referenceFlag & H264PictureContext::used_for_short_term_reference)
        {
            picture->referenceFlag |= H264PictureContext::used_for_short_term_reference;
        }
    }
    else
    {
        uint32_t numShortTerm = 0, numLongTerm = 0;
        for (auto& _picture : pictures)
        {
            if (_picture->referenceFlag & H264PictureContext::used_for_short_term_reference)
            {
                numShortTerm++;
            }
            if (_picture->referenceFlag & H264PictureContext::used_for_long_term_reference)
            {
                numLongTerm++;
            }
        }
        if (numShortTerm + numLongTerm == std::max(1u, sps->max_num_ref_frames))
        {
            H264PictureContext::ptr __picture = nullptr;
            int64_t minFrameNumWrap = INT64_MAX;
            for (auto& _picture : pictures)
            {
                if (_picture->FrameNumWrap < minFrameNumWrap)
                {
                    minFrameNumWrap = _picture->FrameNumWrap;
                    __picture = _picture;
                }
            }
            MPP_H264_SD_LOG("[DRPM] Mark short term picture to unsued FrameNum(%d) FrameNumWrap(%ld)", __picture->FrameNum, __picture->FrameNumWrap);
            __picture->referenceFlag = H264PictureContext::unused_for_reference;
            if (__picture->field_pic_flag == 1)
            {
                // H264PictureContext::ptr compPicture = nullptr;
                // compPicture = FindComplementaryPicture(pictures, __picture);
                // if (!compPicture)
                // {
                //     assert(false);
                //     return;
                // }
                // compPicture->referenceFlag = H264PictureContext::unused_for_reference;
                // Hint : not support for now
                assert(false);
            }     
        }
    }

}

/**
 * @sa ISO 14496/10(2020) - 8.2.5.4 Adaptive memory control decoded reference picture marking process 
 */
void H264SliceDecodingProcess::AdaptiveMemoryControlDecodedReferencePicutreMarkingPorcess(H264SliceHeaderSyntax::ptr slice, H264PictureContext::cache pictures, H264PictureContext::ptr picture)
{
#if ENABLE_MMP_SD_DEBUG
    H26x_LOG_INFO << "AdaptiveMemoryControlDecodedReferencePicutreMarkingPorcess BEGIN";
    {
        H26x_LOG_INFO << "[MM] Short term reference:" << H26x_LOG_TERMINATOR;
        uint32_t index = 0;
        for (auto& __picture : _pictures)
        {
            if (__picture->referenceFlag & H264PictureContext::used_for_short_term_reference)
            {
                H26x_LOG_INFO << "  (" << index << ") FrameNum(" << __picture->FrameNum 
                            << ") TopFieldOrderCnt(" << __picture->TopFieldOrderCnt 
                            << ") BottomFieldOrderCnt(" << __picture->BottomFieldOrderCnt << ")"
                            << " PicNum(" << __picture->PicNum << ")"
                            << H26x_LOG_TERMINATOR;
                index++;
            }
        }
    }
    {
        H26x_LOG_INFO << "[MM] Long term reference:" << H26x_LOG_TERMINATOR;
        uint32_t index = 0;
        for (auto& __picture : _pictures)
        {
            if (__picture->referenceFlag & H264PictureContext::used_for_long_term_reference)
            {
                H26x_LOG_INFO << "  (" << index << ") FrameNum(" << __picture->FrameNum 
                            << ") TopFieldOrderCnt(" << __picture->TopFieldOrderCnt 
                            << ") BottomFieldOrderCnt(" << __picture->BottomFieldOrderCnt << ")"
                            << " LongTermPicNum(" << __picture->LongTermPicNum << ")"
                            << H26x_LOG_TERMINATOR;
                index++;
            }
        }
    }
#endif /* ENABLE_MMP_SD_DEBUG */
    size_t index = 0;
    for (const auto& memory_management_control_operation : slice->drpm->memory_management_control_operations)
    {
        switch (memory_management_control_operation)
        {
            case H264MmcoType::MMP_H264_MMOO_0: /* the end of memory_management_control_operation */
            {
                MPP_H264_SD_LOG("[MM] mmco(%d)", memory_management_control_operation);
                break;
            }
            // See also : 8.2.5.4.1 Marking process of a short-term reference picture as "unused for reference"
            case H264MmcoType::MMP_H264_MMOO_1: /* unmark short term reference */
            {
                uint32_t difference_of_pic_nums_minus1 = slice->drpm->memory_management_control_operations_datas[index++].difference_of_pic_nums_minus1;
                int32_t picNumX = GetPicNumX(slice, difference_of_pic_nums_minus1);
                MPP_H264_SD_LOG("[MM] mmco(%d) difference_of_pic_nums_minus1(%d) picNumX(%d)", memory_management_control_operation, difference_of_pic_nums_minus1, picNumX);
                UnMarkUsedForShortTermReference(pictures, picNumX);
                break;
            }
            // See also : 8.2.5.4.2 Marking process of a long-term reference picture as "unused for reference"
            case H264MmcoType::MMP_H264_MMOO_2: /* unmark long term reference by LongTermPicNum */
            {
                uint32_t long_term_pic_num = slice->drpm->memory_management_control_operations_datas[index++].long_term_pic_num;
                MPP_H264_SD_LOG("[MM] mmco(%d) long_term_pic_num(%d)", memory_management_control_operation, long_term_pic_num);
                UnMarkUsedForLongTermReference(pictures, long_term_pic_num);
                break;
            }
            // See also : 8.2.5.4.3 Assignment process of a LongTermFrameIdx to a short-term reference picture
            case H264MmcoType::MMP_H264_MMOO_3: /* short term reference to long term reference */
            {
                uint32_t difference_of_pic_nums_minus1 = slice->drpm->memory_management_control_operations_datas[index++].difference_of_pic_nums_minus1;
                uint32_t long_term_frame_idx = slice->drpm->memory_management_control_operations_datas[index++].long_term_frame_idx;
                int32_t picNumX = GetPicNumX(slice, difference_of_pic_nums_minus1);
                MPP_H264_SD_LOG("[MM] mmco(%d) difference_of_pic_nums_minus1(%d) long_term_frame_idx(%d) picNumX(%d)", memory_management_control_operation, difference_of_pic_nums_minus1, long_term_frame_idx, picNumX);
                UnMarkUsedForReference(pictures, picture->long_term_frame_idx);
                MarkShortTermReferenceToLongTermReference(pictures, picNumX, long_term_frame_idx);
                break;
            }
            // See also : 8.2.5.4.4 Decoding process for MaxLongTermFrameIdx
            case H264MmcoType::MMP_H264_MMOO_4: /* set maximum long-frame index */
            {
                uint32_t max_long_term_frame_idx_plus1 = slice->drpm->memory_management_control_operations_datas[index++].max_long_term_frame_idx_plus1;
                int64_t MaxLongTermFrameIdx = max_long_term_frame_idx_plus1 == 0 ? no_long_term_frame_indices : max_long_term_frame_idx_plus1 - 1;
                MPP_H264_SD_LOG("[MM] mmco(%d) max_long_term_frame_idx_plus1(%d) MaxLongTermFrameIdx(%ld)", memory_management_control_operation, max_long_term_frame_idx_plus1, MaxLongTermFrameIdx);
                for (auto _picture : pictures)
                {
                    if (_picture->referenceFlag & H264PictureContext::used_for_long_term_reference && _picture->LongTermFrameIdx > max_long_term_frame_idx_plus1 - 1)
                    {
                        _picture->referenceFlag = H264PictureContext::unused_for_reference;
                        _picture->MaxLongTermFrameIdx = MaxLongTermFrameIdx;
                    }
                }
                break;
            }
            // See also : 8.2.5.4.5 Marking process of all reference pictures as "unused for reference" and setting
            //            MaxLongTermFrameIdx to "no long-term frame indices"
            case H264MmcoType::MMP_H264_MMOO_5: /* unmark all reference pictures */
            {
                MPP_H264_SD_LOG("[MM] mmco(%d)", memory_management_control_operation);
                for (auto _picture : pictures)
                {
                    _picture->MaxLongTermFrameIdx = no_long_term_frame_indices;
                    _picture->referenceFlag = H264PictureContext::unused_for_reference;
                }
                picture->has_memory_management_control_operation_5 = true;
                break;
            }
            // See also : 8.2.5.4.6 Process for assigning a long-term frame index to the current picture
            case H264MmcoType::MMP_H264_MMOO_6: /* mark current picture long term */
            {
                uint32_t long_term_frame_idx = slice->drpm->memory_management_control_operations_datas[index++].long_term_frame_idx;
                MPP_H264_SD_LOG("[MM] mmco(%d) long_term_frame_idx(%d)", memory_management_control_operation, long_term_frame_idx);
                picture->referenceFlag = H264PictureContext::used_for_long_term_reference;
                picture->LongTermFrameIdx = long_term_frame_idx;
                break;
            }
            default:
                break;
        }
    }
#if ENABLE_MMP_SD_DEBUG
    H26x_LOG_INFO << "AdaptiveMemoryControlDecodedReferencePicutreMarkingPorcess END";
    {
        H26x_LOG_INFO << "[MM] Short term reference:" << H26x_LOG_TERMINATOR;
        uint32_t index = 0;
        for (auto& __picture : _pictures)
        {
            if (__picture->referenceFlag & H264PictureContext::used_for_short_term_reference)
            {
                H26x_LOG_INFO << "  (" << index << ") FrameNum(" << __picture->FrameNum 
                            << ") TopFieldOrderCnt(" << __picture->TopFieldOrderCnt 
                            << ") BottomFieldOrderCnt(" << __picture->BottomFieldOrderCnt << ")"
                            << " PicNum(" << __picture->PicNum << ")"
                            << H26x_LOG_TERMINATOR;
                index++;
            }

        }
    }
    {
        H26x_LOG_INFO << "[MM] Long term reference:" << H26x_LOG_TERMINATOR;
        uint32_t index = 0;
        for (auto& __picture : _pictures)
        {
            if (__picture->referenceFlag & H264PictureContext::used_for_long_term_reference)
            {
                H26x_LOG_INFO << "  (" << index << ") FrameNum(" << __picture->FrameNum 
                            << ") TopFieldOrderCnt(" << __picture->TopFieldOrderCnt 
                            << ") BottomFieldOrderCnt(" << __picture->BottomFieldOrderCnt << ")"
                            << " LongTermPicNum(" << __picture->LongTermPicNum << ")"
                            << H26x_LOG_TERMINATOR;
                index++;
            }
        }
    }
#endif /* ENABLE_MMP_SD_DEBUG */
}

/*************************************** 8.2.5 Decoded reference picture marking process(End) ******************************************/

H264SliceDecodingProcess::H264SliceDecodingProcess()
{
    _prevPicture = nullptr;
    _curId = 0;
}

H264SliceDecodingProcess::~H264SliceDecodingProcess()
{

}

void H264SliceDecodingProcess::SliceDecodingProcess(H264NalSyntax::ptr nal)
{
    switch (nal->nal_unit_type)
    {
        case H264NaluType::MMP_H264_NALU_TYPE_SPS:
        {
            _spss[nal->sps->seq_parameter_set_id] = nal->sps;
            break;
        }
        case H264NaluType::MMP_H264_NALU_TYPE_PPS:
        {
            _ppss[nal->pps->pic_parameter_set_id] = nal->pps;
            break;
        }
        case H264NaluType::MMP_H264_NALU_TYPE_IDR:
        case H264NaluType::MMP_H264_NALU_TYPE_SLICE:
        {
            H264PictureContext::ptr picture = CreatePictureContext();
            H264SpsSyntax::ptr sps = nullptr;
            H264PpsSyntax::ptr pps = nullptr;
            if (!_ppss.count(nal->slice->pic_parameter_set_id))
            {
                break;
            }
            pps = _ppss[nal->slice->pic_parameter_set_id];
            if (!_spss.count(pps->seq_parameter_set_id))
            {
                break;
            }
            sps = _spss[pps->seq_parameter_set_id];
            {
                picture->field_pic_flag = nal->slice->field_pic_flag;
                picture->bottom_field_flag = nal->slice->bottom_field_flag;
                picture->pic_order_cnt_lsb = nal->slice->pic_order_cnt_lsb;
                picture->FrameNum = (uint32_t)nal->slice->frame_num;
            }
#ifdef ENABLE_MMP_SD_DEBUG
            static uint64_t count = 0;
#endif /* ENABLE_MMP_SD_DEBUG */
            MPP_H264_SD_LOG("[DP] %ld nal_unit_type(%s-%d) slice_type(%s-%d) frame_num(%ld) nal_ref_idc(%d)", 
                count++,
                H264NaluTypeToStr(nal->nal_unit_type).c_str(),
                nal->nal_unit_type, 
                H264SliceTypeToStr(nal->slice->slice_type).c_str(), 
                nal->slice->slice_type, 
                nal->slice->frame_num,
                nal->nal_ref_idc
            );
            OnDecodingBegin();
            DecodingProcessForPictureOrderCount(nal, sps, pps, nal->slice, nal->nal_ref_idc, picture);
            if (nal->nal_unit_type == H264NaluType::MMP_H264_NALU_TYPE_IDR)
            {
                _pictures.clear();
            }
            else if (nal->slice->slice_type == H264SliceType::MMP_H264_P_SLICE ||
                nal->slice->slice_type == H264SliceType::MMP_H264_SP_SLICE ||
                nal->slice->slice_type == H264SliceType::MMP_H264_B_SLICE
            )
            {
                DecodingProcessForReferencePictureListsConstruction(nal->slice, sps, _pictures, picture);
            }
            DecodeReferencePictureMarkingProcess(nal, nal->slice, sps, _pictures, picture, nal->nal_ref_idc);
            OnDecodingEnd();
            picture->id = _curId++;
            _pictures.push_back(picture);
            _prevPicture = picture;
            break;
        }
        default:
            break;
    }
}

H264PictureContext::ptr H264SliceDecodingProcess::GetCurrentPictureContext()
{
    return _prevPicture;
}

H264PictureContext::cache H264SliceDecodingProcess::GetAllPictures()
{
    return _pictures;
}

std::vector<H264PictureContext::ptr> H264SliceDecodingProcess::GetRefPicList0()
{
    return _RefPicList0;
}

std::vector<H264PictureContext::ptr> H264SliceDecodingProcess::GetRefPicList1()
{
    return _RefPicList1;
}

H264PictureContext::ptr H264SliceDecodingProcess::CreatePictureContext()
{
    return std::make_shared<H264PictureContext>();
}

void H264SliceDecodingProcess::OnDecodingBegin()
{
    for (auto& task : _beginTasks)
    {
        task();
    }
    _beginTasks.clear();
}

void H264SliceDecodingProcess::OnDecodingEnd()
{
    for (auto& task : _endTasks)
    {
        task();
    }
    _endTasks.clear();
    H264PictureContext::cache pictures;
    for (auto picture : _pictures)
    {
        if (picture->referenceFlag & H264PictureContext::used_for_short_term_reference || picture->referenceFlag & H264PictureContext::used_for_long_term_reference)
        {
            pictures.push_back(picture);
        }
    }
    _pictures.swap(pictures);
#if ENABLE_MMP_SD_DEBUG
    {
        H26x_LOG_INFO << "Short term reference:" << H26x_LOG_TERMINATOR;
        uint32_t index = 0;
        for (auto& picture : _pictures)
        {
            if (picture->referenceFlag & H264PictureContext::used_for_short_term_reference)
            {
                H26x_LOG_INFO << "  (" << index << ") FrameNum(" << picture->FrameNum 
                            << ") TopFieldOrderCnt(" << picture->TopFieldOrderCnt 
                            << ") BottomFieldOrderCnt(" << picture->BottomFieldOrderCnt << ")"
                            << " PicNum(" << picture->PicNum << ")"
                            << H26x_LOG_TERMINATOR;
            }

        }
    }
    {
        H26x_LOG_INFO << "Long term reference:" << H26x_LOG_TERMINATOR;
        uint32_t index = 0;
        for (auto& picture : _pictures)
        {
            if (picture->referenceFlag & H264PictureContext::used_for_long_term_reference)
            {
                H26x_LOG_INFO << "  (" << index << ") FrameNum(" << picture->FrameNum 
                            << ") TopFieldOrderCnt(" << picture->TopFieldOrderCnt 
                            << ") BottomFieldOrderCnt(" << picture->BottomFieldOrderCnt << ")"
                            << " LongTermPicNum(" << picture->LongTermPicNum << ")"
                            << H26x_LOG_TERMINATOR;
            }
        }
    }
#endif /* ENABLE_MMP_SD_DEBUG */
}

} // namespace Codec
} // namespace Mmp