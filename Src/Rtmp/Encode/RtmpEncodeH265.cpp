#include "RtmpEncodeH265.h"
#include "Codec/H265Frame.h"
#include "Log/Logger.h"
#include "Codec/H265Track.h"
#include "Rtmp/Rtmp.h"
#include "Common/Config.h"

using namespace std;

RtmpEncodeH265::RtmpEncodeH265(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{}

RtmpEncodeH265::~RtmpEncodeH265()
{
}

string RtmpEncodeH265::getConfig()
{
    auto trackInfo = dynamic_pointer_cast<H265Track>(_trackInfo);
    if (!trackInfo->_vps || !trackInfo->_sps || !trackInfo->_pps) {
        return "";
    }

    static bool enbaleEnhanced = Config::instance()->getAndListen([](const json &config){
        enbaleEnhanced = Config::instance()->get("Rtmp", "Server", "Server1", "enableEnhanced");
    }, "Rtmp", "Server", "Server1", "enableEnhanced");

    string config;

    auto spsSize = trackInfo->_sps->startSize();
    auto vpsSize = trackInfo->_vps->startSize();
    auto ppsSize = trackInfo->_pps->startSize();

    auto vpsBuffer = trackInfo->_vps->_buffer;
    int vpsLen = trackInfo->_vps->size() - vpsSize;
    auto spsBuffer = trackInfo->_sps->_buffer;
    int spsLen = trackInfo->_sps->size() - spsSize;
    auto ppsBuffer = trackInfo->_pps->_buffer;
    int ppsLen = trackInfo->_pps->size() - ppsSize;

    config.resize(43  + vpsLen + spsLen + ppsLen);
    auto data = (char*)config.data();

    if (enbaleEnhanced) {
        *data++ = 1 << 7 | 1;
        *data++ = 'h';
        *data++ = 'v';
        *data++ = 'c';
        *data++ = '1';
    } else {
        *data++ = 0x1c; //key frame, AVC
        *data++ = 0x00; //avc sequence header
        *data++ = 0x00; //composit time 
        *data++ = 0x00; //composit time
        *data++ = 0x00; //composit time
    }
    *data++ = 0x01;   //configurationversion
    // 6 byte

    *data++ = spsBuffer[spsSize + 5];  //general_profile_space(2), general_tier_flag(1), general_profile_idc(5)

    *data++ = spsBuffer[spsSize + 6]; //general_profile_compatibility_flags
    *data++ = spsBuffer[spsSize + 7]; 
    *data++ = spsBuffer[spsSize + 8]; 
    *data++ = spsBuffer[spsSize + 9]; 

    *data++ = spsBuffer[spsSize + 10]; //general_constraint_indicator_flags
    *data++ = spsBuffer[spsSize + 11]; 
    *data++ = spsBuffer[spsSize + 12]; 
    *data++ = spsBuffer[spsSize + 13]; 
    *data++ = spsBuffer[spsSize + 14]; 
    *data++ = spsBuffer[spsSize + 15]; 

    *data++ = spsBuffer[spsSize + 16]; // general_level_idc

    *data++ = spsBuffer[spsSize + 17]; // reserved(4),min_spatial_segmentation_idc(12)
    *data++ = spsBuffer[spsSize + 18]; 

    *data++ = spsBuffer[spsSize + 19]; // reserved(6),parallelismType(2)

    *data++ = spsBuffer[spsSize + 20]; // reserved(6),chromaFormat(2)
    // 22 byte 

    // 下面几个0xff乱填的，貌似不影响播放
    *data++ = 0xf << 4 | 0xf; //// reserved(5),bitDepthLumaMinus8(3)

    *data++ = 0xff; // reserved(5),bitDepthChromaMinus8(3)

    *data++ = 0xff; //avgFrameRate
    *data++ = 0xff;

    *data++ = 0xff; //constantFrameRate(2),numTemporalLayers(3),temporalIdNested(1),lengthSizeMinusOne(2)

    // *data++ = 0xff;

    // *data++ = 0xff;
    // *data++ = 0xff;
    // *data++ = 0xff;
    // 默认vps，sps，pps 各一个
    *data++ = 3; //numOfArrays
    //28 byte

    *data++ = 1 << 7 | H265NalType::H265_VPS & 0x3f; //array_completeness(1),reserved(1),NAL_unit_type(6)

    // 一个vps
    *data++ = 0; //numNalus
    *data++ = 1;

    *data++ = (uint8_t)(vpsLen >> 8); //sequence parameter set length high 8 bits
    *data++ = (uint8_t)(vpsLen); //sequence parameter set  length low 8 bits

    memcpy(data, vpsBuffer.data() + vpsSize, vpsLen); //H264 sequence parameter set
    data += vpsLen; 
    // 37 + vpsLen

    *data++ = 1 << 7 | H265NalType::H265_SPS & 0x3f;

    // 一个sps
    *data++ = 0;
    *data++ = 1;

    *data++ = (uint8_t)(spsLen >> 8); //sequence parameter set length high 8 bits
    *data++ = (uint8_t)(spsLen); //sequence parameter set  length low 8 bits

    memcpy(data, spsBuffer.data() + spsSize, spsLen); //H264 sequence parameter set
    data += spsLen; 
    // 38 + vpsLen + spsLen

    *data++ = 1 << 7 | H265NalType::H265_PPS & 0x3f;

    // 一个pps
    *data++ = 0;
    *data++ = 1;

    *data++ = (uint8_t)(ppsLen >> 8); //sequence parameter set length high 8 bits
    *data++ = (uint8_t)(ppsLen); //sequence parameter set  length low 8 bits

    memcpy(data, ppsBuffer.data() + ppsSize, ppsLen); //H264 sequence parameter set
    data += ppsLen; 
    // 43  + vpsLen + spsLen + ppsLen

    return config;
}

void RtmpEncodeH265::encode(const FrameBuffer::Ptr& frame)
{
    static bool enbaleEnhanced = Config::instance()->getAndListen([](const json &config){
        enbaleEnhanced = Config::instance()->get("Rtmp", "Server", "Server1", "enableEnhanced");
    }, "Rtmp", "Server", "Server1", "enableEnhanced");

    int enhancedExtraSize = 0;
    if (enbaleEnhanced) {
        enhancedExtraSize = 3;
    }

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
                auto h265It = dynamic_pointer_cast<H265Frame>(it);
                auto keyFrame = h265It->keyFrame();
                int cts = it->pts() - it->dts();
                int length = it->size() - it->startSize();
                
                if (h265It->startFrame()) {
                    start = true;
                }

                if (first) {
                    if (enbaleEnhanced) {
                        uint8_t frameType = keyFrame ? 1 : 2;
                        *data++ = 1 << 7 | frameType << 4 | 1;
                        *data++ = 'h';
                        *data++ = 'v';
                        *data++ = 'c';
                        *data++ = '1';
                        *data++ = (uint8_t)(cts >> 16); 
                        *data++ = (uint8_t)(cts >> 8); 
                        *data++ = (uint8_t)(cts); 
                        index += 8;
                    } else {
                        if (keyFrame) {
                            *data++ = 0x1c;
                        } else {
                            *data++ = 0x2c;
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

    
    auto h264Frame = dynamic_pointer_cast<H265Frame>(frame);
    auto nalType = h264Frame->getNalType();
    int length = frame->size() - frame->startSize();

    if (nalType == H265_PPS || nalType == H265_SPS 
        || nalType == H265_SEI_PREFIX || nalType == H265_SEI_SUFFIX || nalType == H265_VPS) {
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

void RtmpEncodeH265::setOnRtmpPacket(const function<void(const RtmpMessage::Ptr& msg, bool start)>& cb)
{
    _onRtmpMessage = cb;
}

void RtmpEncodeH265::onRtmpMessage(const RtmpMessage::Ptr& msg, bool start)
{
    if (_onRtmpMessage) {
        _onRtmpMessage(msg, start);
    }
}
