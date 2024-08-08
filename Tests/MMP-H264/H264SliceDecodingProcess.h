//
// H264SliceDecodingProcess.h
//
// Library: Codec
// Package: H264
// Module:  H264
// 

#include "H264Common.h"

#include <functional>

namespace Mmp
{
namespace Codec
{

class H264PictureContext
{
public:
    using ptr = std::shared_ptr<H264PictureContext>;
public:
    using cache = std::vector<H264PictureContext::ptr>;
public:
    H264PictureContext();
    virtual ~H264PictureContext() = default;
public:
    static constexpr uint64_t unused_for_reference = 0;
    static constexpr uint64_t used_for_short_term_reference = 1 << 0U;
    static constexpr uint64_t used_for_long_term_reference = 1 << 1U;
    static constexpr uint64_t non_existing = 1 << 2U;
public:
    uint64_t id;
public: /* inherit from nal unit */
    uint8_t   field_pic_flag;
    uint8_t   bottom_field_flag;
    uint8_t   pic_order_cnt_lsb;
    uint32_t  long_term_frame_idx;
    bool      has_memory_management_control_operation_5;
public: /* 8.2.1 Decoding process for picture order count */
    int32_t  TopFieldOrderCnt;
    int32_t  BottomFieldOrderCnt;
    int32_t  prevPicOrderCntMsb;
    int64_t  FrameNumOffset;
public: /* 8.2.4 Decoding process for reference picture lists construction */
    uint64_t MaxFrameNum;
    uint32_t FrameNum;
    int64_t  FrameNumWrap;
    int64_t  PicNum;
public: /* 8.2.5 Decoded reference picture marking process */
    uint64_t referenceFlag;
    int64_t  MaxLongTermFrameIdx;
    int64_t  LongTermFrameIdx;
    uint32_t LongTermPicNum;
};

/**
 * @sa  8.2 Slice decoding process - ISO 14496/10(2020)
 */
class H264SliceDecodingProcess
{
public:
    using ptr = std::shared_ptr<H264SliceDecodingProcess>;
public:
    H264SliceDecodingProcess();
    virtual ~H264SliceDecodingProcess();
public:
    void SliceDecodingProcess(H264NalSyntax::ptr nal);
public:
    H264PictureContext::ptr GetCurrentPictureContext();
    H264PictureContext::cache GetAllPictures();
    std::vector<H264PictureContext::ptr> GetRefPicList0();
    std::vector<H264PictureContext::ptr> GetRefPicList1();
protected:
    virtual H264PictureContext::ptr CreatePictureContext();
private:
    using task = std::function<void()>;
private:
    void OnDecodingBegin();
    void OnDecodingEnd();
private:
    void DecodingProcessForPictureOrderCount(H264NalSyntax::ptr nal, H264SpsSyntax::ptr sps, H264PpsSyntax::ptr pps, H264SliceHeaderSyntax::ptr slice, uint8_t nal_ref_idc, H264PictureContext::ptr picture);
    void DecodeH264PictureOrderCountType0(H264PictureContext::ptr prevPictrue, H264SpsSyntax::ptr sps, H264PpsSyntax::ptr pps, H264SliceHeaderSyntax::ptr slice, uint8_t nal_ref_idc, H264PictureContext::ptr picture);
    void DecodeH264PictureOrderCountType1(H264PictureContext::ptr prevPictrue, H264NalSyntax::ptr nal, H264SpsSyntax::ptr sps, H264SliceHeaderSyntax::ptr slice, uint8_t nal_ref_idc, H264PictureContext::ptr picture);
    void DecodeH264PictureOrderCountType2(H264PictureContext::ptr prevPictrue, H264SpsSyntax::ptr sps, H264SliceHeaderSyntax::ptr slice, uint8_t nal_ref_idc, H264PictureContext::ptr picture);
private:
    void DecodingProcessForReferencePictureListsConstruction(H264SliceHeaderSyntax::ptr slice, H264SpsSyntax::ptr sps, H264PictureContext::cache pictures, H264PictureContext::ptr picture);
    void DecodingProcessForPictureNumbers(H264SliceHeaderSyntax::ptr slice, H264SpsSyntax::ptr sps, H264PictureContext::cache pictures, H264PictureContext::ptr picture);
    void InitializationProcessForReferencePictureLists(H264SliceHeaderSyntax::ptr slice, H264PictureContext::cache pictures, H264PictureContext::ptr picture);
    void ModificationProcessForReferencePictureLists(H264SliceHeaderSyntax::ptr slice, H264SpsSyntax::ptr sps, H264PictureContext::cache pictures, H264PictureContext::ptr picture);
private:
    void DecodeReferencePictureMarkingProcess(H264NalSyntax::ptr nal, H264SliceHeaderSyntax::ptr slice, H264SpsSyntax::ptr sps, H264PictureContext::cache pictures, H264PictureContext::ptr picture, uint8_t nal_ref_idc);
    void DecodingProcessForGapsInFrameNum(H264SliceHeaderSyntax::ptr slice, H264SpsSyntax::ptr sps, H264PictureContext::ptr picture, uint64_t PrevRefFrameNum);
    void SequenceOfOperationsForDecodedReferencePictureMarkingProcess(H264NalSyntax::ptr nal, H264SliceHeaderSyntax::ptr slice, H264SpsSyntax::ptr sps, H264PictureContext::cache pictures, H264PictureContext::ptr picture);
    void SlidingWindowDecodedReferencePictureMarkingProcess(H264SliceHeaderSyntax::ptr slice, H264SpsSyntax::ptr sps, H264PictureContext::cache pictures, H264PictureContext::ptr picture);
    void AdaptiveMemoryControlDecodedReferencePicutreMarkingPorcess(H264SliceHeaderSyntax::ptr slice, H264PictureContext::cache pictures, H264PictureContext::ptr picture);
private:
    H264PictureContext::ptr _prevPicture;
private:
    std::vector<H264PictureContext::ptr> _RefPicList0;
    std::vector<H264PictureContext::ptr> _RefPicList1;
private:
    uint64_t _curId;
    H264PictureContext::cache _pictures;
    std::map<uint32_t, H264SpsSyntax::ptr> _spss;
    std::map<uint32_t, H264PpsSyntax::ptr> _ppss;
private:
    std::vector<task> _beginTasks;
    std::vector<task> _endTasks;
};

} // namespace Codec
} // namespace Mmp