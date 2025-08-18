#ifndef H266Frame_H
#define H266Frame_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Common/Frame.h"

using namespace std;

enum H266NalType
{
    H266_IDR_W_RADL = 7,
    H266_RSV_IRAP = 11,
    H266_OPI = 12,
    H266_DCI = 13,
    H266_VPS = 14,
    H266_SPS = 15,
    H266_PPS = 16,
    H266_PREFIX_APS_NUT = 17,
    H266_SUFFIX_APS_NUT = 18,
    H266_PH_NUT = 19,
    H266_AUD = 20,
    H266_PREFIX_SEI = 23,
    H266_SUFFIX_SEI = 24,
};

class H266Frame : public FrameBuffer
{
public:
    using Ptr = shared_ptr<H266Frame>;

    H266Frame()
    {
        _codec = "h266";
    }
    
    H266Frame(const H266Frame::Ptr& frame);

    bool isNewNalu() override;

    bool keyFrame() const override
    {
        uint8_t type = (uint8_t)(_buffer[_startSize + 1]) >> 3;
        return type >= H266_IDR_W_RADL && type < H266_RSV_IRAP;
    }

    bool metaFrame() const override
    {
        uint8_t type = (uint8_t)(_buffer[_startSize + 1]) >> 3;
        switch(type){
            case H266NalType::H266_VPS:
            case H266NalType::H266_SPS:
            case H266NalType::H266_PPS : return true;
            default : return false;
        }
    }

    bool startFrame() const override
    {
        uint8_t type = (uint8_t)(_buffer[_startSize + 1]) >> 3;
        if (type == H266NalType::H266_VPS) {
            return true;
        }

        return false;
    }

    uint8_t getNalType() override
    {
        return (uint8_t)(_buffer[_startSize + 1]) >> 3;
    }

    bool isNonPicNalu() override
    {
        uint8_t type = getNalType();
        switch(type){
            // case H266NalType::H266_VPS:
            // case H266NalType::H266_SPS:
            // case H266NalType::H266_PPS:
            case H266NalType::H266_AUD:
            case H266NalType::H266_PREFIX_SEI:
            case H266NalType::H266_SUFFIX_SEI: 
                return true;
            default : return false;
        }
    }

    static uint8_t getNalType(uint8_t nalByte)
    {
        return nalByte >> 3;
    }

    void split(const function<void(const FrameBuffer::Ptr& frame)>& cb) override;
    static FrameBuffer::Ptr createFrame(int startSize, int index, bool addStart);
    
    static void registerFrame();
};


#endif //H266Frame_H
