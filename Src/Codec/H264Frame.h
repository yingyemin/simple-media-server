#ifndef H264Frame_H
#define H264Frame_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Common/Frame.h"

// using namespace std;

enum H264NalType
{
    H264_BP = 1,
    H264_DataPartitionA = 2,
    H264_IDR = 5,
    H264_SEI = 6,
    H264_SPS = 7,
    H264_PPS = 8,
    H264_AUD = 9
};

enum H264SliceType
{
    H264_SliceType_P   = 0,
    H264_SliceType_B   = 1,
    H264_SliceType_I   = 2,
    H264_SliceType_SP  = 3,
    H264_SliceType_SI  = 4,
    H264_SliceType_P1  = 5,
    H264_SliceType_B1  = 6,
    H264_SliceType_I1  = 7,
    H264_SliceType_SP1 = 8,
    H264_SliceType_SI1 = 9,
};

class H264Frame : public FrameBuffer
{
public:
    using Ptr = std::shared_ptr<H264Frame>;

    H264Frame()
    {
        _codec = "h264";
    }

    H264Frame(const H264Frame::Ptr& frame);

    bool keyFrame() const override
    {
        uint8_t type = (uint8_t)((*_buffer)[_startSize]) & 0x1F;
        return type == H264NalType::H264_IDR;
    }

    bool metaFrame() const override
    {
        uint8_t type = (uint8_t)((*_buffer)[_startSize]) & 0x1F;
        switch(type){
            case H264NalType::H264_SPS:
            case H264NalType::H264_PPS:
                return true;
            default:
                return false;
        }
    }

    bool startFrame() const override
    {
        uint8_t type = (uint8_t)((*_buffer)[_startSize]) & 0x1F;
        if (type == H264NalType::H264_SPS) {
            return true;
        }

        return false;
    }

    uint8_t getNalType() override
    {
        return (uint8_t)((*_buffer)[_startSize]) & 0x1F;
    }

    bool isBFrame() override;

    bool isNewNalu() override;

    bool isNonPicNalu() override
    {
        uint8_t type = (uint8_t)((*_buffer)[_startSize]) & 0x1F;
        switch(type){
            // case H264NalType::H264_SPS:
            // case H264NalType::H264_PPS:
            case H264NalType::H264_AUD:
            case H264NalType::H264_SEI:
                return true;
            default : return false;
        }
    }

    static uint8_t getNalType(uint8_t nalByte)
    {
        return nalByte & 0x1F;
    }

    void split(const std::function<void(const FrameBuffer::Ptr& frame)>& cb) override;
    
    static FrameBuffer::Ptr createFrame(int startSize, int index, bool addStart);
    
    static void registerFrame();
};


#endif //H264Frame_H