﻿#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "H265Frame.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Config.h"

using namespace std;

H265Frame::H265Frame(const H265Frame::Ptr& frame)
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

void H265Frame::split(const function<void(const FrameBuffer::Ptr& frame)>& cb)
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
        if (!(alwaysSplit || type == H265_VPS || type == H265_SPS || type == H265_PPS
                || type == H265_SEI_PREFIX  || type == H265_SEI_SUFFIX))
        {
            break;
        }

        auto next_start = findNextNalu(start + prefix, end - start - prefix, next_prefix);
        if (next_start) {
            //找到下一帧
            
            // TODO 目前先拷贝内存，后续直接复用frame的内存，只是改一下offset和length
            H265Frame::Ptr subFrame = make_shared<H265Frame>(dynamic_pointer_cast<H265Frame>(shared_from_this()));
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
    H265Frame::Ptr subFrame = make_shared<H265Frame>(dynamic_pointer_cast<H265Frame>(shared_from_this()));
    subFrame->_startSize = prefix;
    subFrame->_buffer.assign(start, end - start);

    cb(subFrame);
}

bool H265Frame::isNewNalu()
{
    if (size() < 4) {
        return false;
    }

    auto nalu = data() + startSize();
    auto bytes = size() - startSize();

    uint8_t nal_type;
    uint8_t nuh_layer_id;
    
    if(bytes < 3)
        return false;
    
    nal_type = (nalu[0] >> 1) & 0x3f;
    nuh_layer_id = ((nalu[0] & 0x01) << 5) | ((nalu[1] >> 3) &0x1F);
    
    // 7.4.2.4.4 Order of NAL units and coded pictures and their association to access units
    if(H265_VPS == nal_type || H265_SPS == nal_type || H265_PPS == nal_type ||
       (nuh_layer_id == 0 && (H265_AUD == nal_type || H265_SEI_PREFIX == nal_type || (41 <= nal_type && nal_type <= 44) || (48 <= nal_type && nal_type <= 55))))
        return 1;
        
    // 7.4.2.4.5 Order of VCL NAL units and association to coded pictures
    if (nal_type <= 31)
    {
        //first_slice_segment_in_pic_flag 0x80
        return (nalu[2] & 0x80) ? 1 : 0;
    }
    
    return 0;
}

FrameBuffer::Ptr H265Frame::createFrame(int startSize, int index, bool addStart)
{
    auto frame = make_shared<H265Frame>();
        
    frame->_startSize = startSize;
    frame->_codec = "h265";
    frame->_index = index;
    frame->_trackType = 0;//VideoTrackType;

    if (addStart) {
        frame->_buffer.assign("\x00\x00\x00\x01", 4);
        frame->_startSize = 4;
    };

    return frame;
}

void H265Frame::registerFrame()
{
	FrameBuffer::registerFrame("h265", H265Frame::createFrame);
}