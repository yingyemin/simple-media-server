#include "RtmpEncodeH266.h"
#include "Codec/H266Frame.h"
#include "Log/Logger.h"
#include "Codec/H266Track.h"
#include "Rtmp/Rtmp.h"
#include "Common/Config.h"

using namespace std;

RtmpEncodeH266::RtmpEncodeH266(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{}

RtmpEncodeH266::~RtmpEncodeH266()
{
}

string RtmpEncodeH266::getConfig()
{
    auto trackInfo = dynamic_pointer_cast<H266Track>(_trackInfo);
    if (!trackInfo->_sps || !trackInfo->_pps) {
        return "";
    }

    // static bool enbaleEnhanced = Config::instance()->getAndListen([](const json &config){
    //     enbaleEnhanced = Config::instance()->get("Rtmp", "Server", "Server1", "enableEnhanced");
    // }, "Rtmp", "Server", "Server1", "enableEnhanced");

    string config;

    config.resize(5);
    auto data = (char*)config.data();

    if (_enhanced) {
        *data++ = 1 << 7 | 1  << 4;
        *data++ = 'v';
        *data++ = 'v';
        *data++ = 'c';
        *data++ = '1';
    } else {
        *data++ = 0x1e; //key frame, AVC
        *data++ = 0x00; //avc sequence header
        *data++ = 0x00; //composit time 
        *data++ = 0x00; //composit time
        *data++ = 0x00; //composit time
    }
    
    // version and flag
    // *data++ = 0x00;
    // *data++ = 0x00;
    // *data++ = 0x00;
    // *data++ = 0x00;

    config.append(trackInfo->getConfig());

    return config;
}

void RtmpEncodeH266::encode(const FrameBuffer::Ptr& frame)
{
    // static bool enbaleEnhanced = Config::instance()->getAndListen([](const json &config){
    //     enbaleEnhanced = Config::instance()->get("Rtmp", "Server", "Server1", "enableEnhanced");
    // }, "Rtmp", "Server", "Server1", "enableEnhanced");

    int enhancedExtraSize = 0;
    if (_enhanced) {
        enhancedExtraSize = 3;
    }

    // if (frame->getNalType() == H265_AUD || frame->getNalType() == H265_SEI_PREFIX) {
    //     return ;
    // }

    auto msg = make_shared<RtmpMessage>();

    if (!_append && !_vecFrame.empty()) {
        if (_lastStamp != frame->pts() || frame->startSize() > 0) {
            auto msg = make_shared<RtmpMessage>();
            msg->payload = make_shared<StreamBuffer>(_msgLength + 1);
            
            int index = 0;
            bool first = true;
            bool start = false;
            for (auto& it : _vecFrame) {
                auto data = msg->payload->data() + index;
                auto h266It = dynamic_pointer_cast<H266Frame>(it);
                auto keyFrame = h266It->keyFrame() || h266It->metaFrame();
                int cts = it->pts() - it->dts();
                int length = it->size() - it->startSize();
                
                if (h266It->startFrame()) {
                    start = true;
                }

                if (first) {
                    if (_enhanced) {
                        uint8_t frameType = keyFrame ? 1 : 2;
                        *data++ = 1 << 7 | frameType << 4 | 1;
                        *data++ = 'v';
                        *data++ = 'v';
                        *data++ = 'c';
                        *data++ = '1';
                        *data++ = (uint8_t)(cts >> 16); 
                        *data++ = (uint8_t)(cts >> 8); 
                        *data++ = (uint8_t)(cts); 
                        index += 8;
                    } else {
                        if (keyFrame) {
                            *data++ = 0x1e;
                        } else {
                            *data++ = 0x2e;
                        }
                        *data++ = 1;
                        *data++ = (uint8_t)(cts >> 16); 
                        *data++ = (uint8_t)(cts >> 8); 
                        *data++ = (uint8_t)(cts); 

                        index += 5;
                    }

                    first = false;
                }
                *data++ = (uint8_t)(length >> 24); 
                *data++ = (uint8_t)(length >> 16); 
                *data++ = (uint8_t)(length >> 8); 
                *data++ = (uint8_t)(length);

                memcpy(data, it->data() + it->startSize(), length);

                index += 4 + length;
            }

            msg->abs_timestamp = _lastStamp;
            msg->trackIndex_ = _trackInfo->index_;
            msg->length = _msgLength;
            msg->type_id = RTMP_VIDEO;
            msg->csid = RTMP_CHUNK_VIDEO_ID;

            onRtmpMessage(msg, start);

            _msgLength = 0;
            _vecFrame.clear();
        }
    }

    
    auto h266Frame = dynamic_pointer_cast<H266Frame>(frame);
    auto nalType = h266Frame->getNalType();
    int length = frame->size() - frame->startSize();

    if (nalType == H266_PPS || nalType == H266_SPS 
        || nalType == H266_PREFIX_APS_NUT || nalType == H266_SUFFIX_APS_NUT || nalType == H266_VPS) {
        _append = true;
    } else {
        _append = false;
    }

    if (_vecFrame.empty()) {
        _msgLength += 9 + length + enhancedExtraSize;
    } else {
        _msgLength += 4 + length;
    }
    _vecFrame.push_back(frame);

    _lastStamp = frame->pts();
}



// void RtmpEncodeH265::encode(const FrameBuffer::Ptr& frame)
// {
    
//     auto h265Frame = dynamic_pointer_cast<H265Frame>(frame);
//     auto keyFrame = h265Frame->keyFrame();
//     auto nalType = h265Frame->getNalType();

//     bool isMetaFrame = false;
//     if (nalType == H265_PPS || nalType == H265_SPS 
//         || nalType == H265_SEI_PREFIX || nalType == H265_SEI_SUFFIX || nalType == H265_VPS) {
//         isMetaFrame = true;
//     }

//     int length = frame->size() - frame->startSize();
//     auto msg = make_shared<RtmpMessage>();
//     if (_append || (!_first && _lastStamp == frame->pts())) {
        
//     } 

//     int cts = frame->pts() - frame->dts();
//     msg->payload.reset(new char[9 + length]);
//     auto data = msg->payload.get();
//     if (keyFrame) {
//         *data++ = 0x1c;
//     } else {
//         *data++ = 0x2c;
//     }
//     *data++ = 1;
//     *data++ = (uint8_t)(cts >> 16); 
//     *data++ = (uint8_t)(cts >> 8); 
//     *data++ = (uint8_t)(cts); 

    
//     *data++ = (uint8_t)(length >> 24); 
//     *data++ = (uint8_t)(length >> 16); 
//     *data++ = (uint8_t)(length >> 8); 
//     *data++ = (uint8_t)(length);

//     memcpy(data, frame->data() + frame->startSize(), length);

//     msg->abs_timestamp = frame->pts();
//     msg->trackIndex_ = frame->_index;
//     msg->length = 9 + length;
//     msg->type_id = RTMP_VIDEO;
//     msg->csid = RTMP_CHUNK_VIDEO_ID;

//     onRtmpMessage(msg);

//     _lastStamp = frame->pts();
//     if (isMetaFrame) {
//         _append = true;
//     } else {
//         _append = false;
//     }
//     if (_first) {
//         _first = false;
//     }
// }

void RtmpEncodeH266::setOnRtmpPacket(const function<void(const RtmpMessage::Ptr& msg, bool start)>& cb)
{
    _onRtmpMessage = cb;
}

void RtmpEncodeH266::onRtmpMessage(const RtmpMessage::Ptr& msg, bool start)
{
    if (_onRtmpMessage) {
        _onRtmpMessage(msg, start);
    }
}
