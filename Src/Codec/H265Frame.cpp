#include <cstdlib>
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
    _codec = frame->_codec;
    _trackType = frame->_trackType;
    _pts = frame->_pts;
    _profile = frame->_profile;
    _index = frame->_index;
    _dts = frame->_dts;
}

void H265Frame::split(const function<void(const FrameBuffer::Ptr& frame)>& cb)
{
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
    
    //未找到下一帧,这是最后一帧
    H265Frame::Ptr subFrame = make_shared<H265Frame>(dynamic_pointer_cast<H265Frame>(shared_from_this()));
    subFrame->_startSize = prefix;
    subFrame->_buffer.assign(start, end - start);

    cb(subFrame);
}