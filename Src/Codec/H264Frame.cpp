#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "H264Frame.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Config.h"
#include "H264Nal.h"
#include "Log/Logger.h"

using namespace std;

H264Frame::H264Frame(const H264Frame::Ptr& frame)
{
    if (frame) {
        _codec = frame->_codec;
        _trackType = frame->_trackType;
        _pts = frame->_pts;
        _profile = frame->_profile;
        _index = frame->_index;
        _dts = frame->_dts;
    }
}

bool H264Frame::isBFrame()
{
    if (_buffer.size() <= 5) {
        return false;
    }

    auto nalType = getNalType();
    if (nalType != H264_BP && nalType != H264_IDR && nalType != H264_DataPartitionA) {
        return false;
    }
    
    auto data = _buffer.data() + 5;
    auto len = _buffer.size() - 5;

    uint32_t startBit = 0;
    auto first_mb_in_slice = Ue((unsigned char*)data, len, startBit);
    auto sliceType = Ue((unsigned char*)data, len, startBit);

    // logInfo << "get a  slice type: " << (int)sliceType;

    if (sliceType == H264_SliceType_B1 || sliceType == H264_SliceType_B) {
        return true;
    }

    return false;
}

bool H264Frame::isNewNalu()
{ 
    if (size() < 4) {
        return false;
    }

    auto nalu = data() + startSize();
    auto bytes = size() - startSize();
    uint8_t nal_type;
    
    if(bytes < 2)
        return false;
    
    nal_type = nalu[0] & 0x1f;
    
    // 7.4.1.2.3 Order of NAL units and coded pictures and association to access units
    if(H264_AUD == nal_type || H264_SPS == nal_type || H264_PPS == nal_type || H264_SEI == nal_type || (14 <= nal_type && nal_type <= 18))
        return 1;
    
    // 7.4.1.2.4 Detection of the first VCL NAL unit of a primary coded picture
    if(H264_IDR == nal_type || H264_DataPartitionA == nal_type || H264_BP == nal_type)
    {
        // Live555 H264or5VideoStreamParser::parse
        // The high-order bit of the byte after the "nal_unit_header" tells us whether it's
        // the start of a new 'access unit' (and thus the current NAL unit ends an 'access unit'):
        return (nalu[1] & 0x80) ? 1 : 0; // first_mb_in_slice
    }
    
    return 0;
}

void H264Frame::split(const function<void(const FrameBuffer::Ptr& frame)>& cb)
{
    if (!cb) {
        return ;
    }

    if (size() < 4) {
        cb(shared_from_this());
        return ;
    }

    auto ptr = _buffer.data();
    auto prefix = _startSize;
    
    const char* start = ptr;
    auto end = ptr + _buffer.size();
    size_t next_prefix;

    static int alwaysSplit = Config::instance()->getAndListen([](const json &config){
        alwaysSplit = Config::instance()->get("alwaysSplit");
    }, "alwaysSplit");

    while (true) {
        int type = getNalType(*(start + prefix));
        if (!(alwaysSplit || type == H264_SPS || type == H264_PPS
                || type == H264_SEI))
        {
            break;
        }
        
        auto next_start = findNextNalu(start + prefix, end - start - prefix, next_prefix);
        if (next_start) {
            //找到下一帧
            
            // TODO 目前先拷贝内存，后续直接复用frame的内存，只是改一下offset和length
            H264Frame::Ptr subFrame = make_shared<H264Frame>(dynamic_pointer_cast<H264Frame>(shared_from_this()));
            subFrame->_startSize = prefix;
            subFrame->_buffer.assign(start, next_start - start);

            // cb(start - prefix, next_start - start + prefix, prefix);
            cb(subFrame);

            //搜索下一帧末尾的起始位置
            start = next_start;
            //记录下一帧的prefix长度
            prefix = next_prefix;
        } else {
            break;
        }
    }

    if (ptr == start) {
        cb(shared_from_this());
        return ;
    }
    
    //未找到下一帧,这是最后一帧
    H264Frame::Ptr subFrame = make_shared<H264Frame>(dynamic_pointer_cast<H264Frame>(shared_from_this()));
    subFrame->_startSize = prefix;
    subFrame->_buffer.assign(start, end - start);

    cb(subFrame);
}