#ifndef H265Frame_H
#define H265Frame_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Common/Frame.h"

using namespace std;

enum H265NalType
{
    H265_TRAIL_N = 0,
    H265_TRAIL_R = 1,
    H265_TSA_N = 2,
    H265_TSA_R = 3,
    H265_STSA_N = 4,
    H265_STSA_R = 5,
    H265_RADL_N = 6,
    H265_RADL_R = 7,
    H265_RASL_N = 8,
    H265_RASL_R = 9,
    
    H265_BLA_W_LP = 16,
    H265_BLA_W_RADL = 17,
    H265_BLA_N_LP = 18,
    H265_IDR_W_RADL = 19,
    H265_IDR_N_LP = 20,
    H265_CRA_NUT = 21,
    H265_RSV_IRAP_VCL22 = 22,
    H265_RSV_IRAP_VCL23 = 23,

    H265_VPS = 32,
    H265_SPS = 33,
    H265_PPS = 34,
    H265_AUD = 35,
    H265_EOS_NUT = 36,
    H265_EOB_NUT = 37,
    H265_FD_NUT = 38,
    H265_SEI_PREFIX = 39,
    H265_SEI_SUFFIX = 40,
};

class H265Frame : public FrameBuffer
{
public:
    using Ptr = shared_ptr<H265Frame>;

    H265Frame()
    {
        _codec = "h265";
    }
    
    H265Frame(const H265Frame::Ptr& frame);

    bool isNewNalu() override;

    bool keyFrame() const override
    {
        uint8_t type = ((uint8_t)(_buffer[_startSize]) >> 1) & 0x3f;
        return type >= H265NalType::H265_BLA_W_LP && type <= H265NalType::H265_RSV_IRAP_VCL23;
    }

    bool metaFrame() const override
    {
        uint8_t type = ((uint8_t)(_buffer[_startSize]) >> 1) & 0x3f;
        switch(type){
            case H265NalType::H265_VPS:
            case H265NalType::H265_SPS:
            case H265NalType::H265_PPS : return true;
            default : return false;
        }
    }

    bool startFrame() const override
    {
        uint8_t type = ((uint8_t)(_buffer[_startSize]) >> 1) & 0x3f;
        if (type == H265NalType::H265_VPS) {
            return true;
        }

        return false;
    }

    uint8_t getNalType() override
    {
        return ((uint8_t)(_buffer[_startSize]) >> 1) & 0x3f;
    }

    static uint8_t getNalType(uint8_t nalByte)
    {
        return nalByte >> 1 & 0x3f;
    }

    void split(const function<void(const FrameBuffer::Ptr& frame)>& cb) override;
};


#endif //H265Frame_H
