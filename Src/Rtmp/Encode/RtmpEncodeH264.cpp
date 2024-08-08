#include "RtmpEncodeH264.h"
#include "Logger.h"
#include "Codec/H264Track.h"
#include "Codec/H264Frame.h"
#include "Rtmp/Rtmp.h"

using namespace std;

RtmpEncodeH264::RtmpEncodeH264(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{}

RtmpEncodeH264::~RtmpEncodeH264()
{
}

string RtmpEncodeH264::getConfig()
{
    auto trackInfo = dynamic_pointer_cast<H264Track>(_trackInfo);
    if (!trackInfo->_sps || !trackInfo->_pps) {
        return "";
    }

    string config;

    auto spsSize = trackInfo->_sps->startSize();
    auto ppsSize = trackInfo->_pps->startSize();

    auto spsBuffer = trackInfo->_sps->_buffer;
    int spsLen = trackInfo->_sps->size() - spsSize;
    auto ppsBuffer = trackInfo->_pps->_buffer;
    int ppsLen = trackInfo->_pps->size() - ppsSize;

    config.resize(16 + spsLen + ppsLen);
    auto data = (char*)config.data();

    *data++ = 0x17; //key frame, AVC
    *data++ = 0x00; //avc sequence header
    *data++ = 0x00; //composit time 
    *data++ = 0x00; //composit time
    *data++ = 0x00; //composit time
    *data++ = 0x01;   //configurationversion
    *data++ = spsBuffer[spsSize + 1]; //avcprofileindication
    *data++ = spsBuffer[spsSize + 2]; //profilecompatibilty
    *data++ = spsBuffer[spsSize + 3]; //avclevelindication
    *data++ = 0xff;   //reserved + lengthsizeminusone
    *data++ = 0xe1;   //num of sps
    *data++ = (uint8_t)(spsLen >> 8); //sequence parameter set length high 8 bits
    *data++ = (uint8_t)(spsLen); //sequence parameter set  length low 8 bits
    // data length 13
    memcpy(data, spsBuffer.data() + spsSize, spsLen); //H264 sequence parameter set
    data += spsLen; // 13 + spsLen

    *data++ = 0x01; //num of pps
    *data++ = (uint8_t)(ppsLen >> 8); //picture parameter set length high 8 bits
    *data++ = (uint8_t)(ppsLen); //picture parameter set length low 8 bits
    // 16 + spsLen + ppsLen
    memcpy(data, ppsBuffer.data() + ppsSize, ppsLen); //H264 picture parameter set

    return config;
}

void RtmpEncodeH264::encode(const FrameBuffer::Ptr& frame)
{
    // FILE* fp = fopen("testrtmpvod.h264", "ab+");
    // fwrite(frame->data(), 1, frame->size(), fp);
    // fclose(fp);
//     auto h264Frame = dynamic_pointer_cast<H264Frame>(frame);
//     // auto keyFrame = h264Frame->keyFrame();
//     auto nalType = h264Frame->getNalType();

//     int length = frame->size() - frame->startSize();
    auto msg = make_shared<RtmpMessage>();

    if (!_append && !_vecFrame.empty()) { 
        if (_lastStamp != frame->pts() || frame->startSize() > 0) {
            auto msg = make_shared<RtmpMessage>();
            msg->payload.reset(new char[_msgLength], [](char* p){delete[] p;});
            
            int index = 0;
            bool first = true;
            bool start = false;
            for (auto& it : _vecFrame) {
                auto data = msg->payload.get() + index;
                auto h264It = dynamic_pointer_cast<H264Frame>(it);
                auto keyFrame = h264It->keyFrame() || h264It->metaFrame();
                int cts = it->pts() - it->dts();
                int length = it->size() - it->startSize();

                if (h264It->startFrame()) {
                    start = true;
                }

                if (first) {
                    if (keyFrame) {
                        *data++ = 0x17;
                    } else {
                        *data++ = 0x27;
                    }
                    *data++ = 1;
                    *data++ = (uint8_t)(cts >> 16); 
                    *data++ = (uint8_t)(cts >> 8); 
                    *data++ = (uint8_t)(cts); 

                    index += 5;

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

    
    auto h264Frame = dynamic_pointer_cast<H264Frame>(frame);
    auto nalType = h264Frame->getNalType();
    int length = frame->size() - frame->startSize();

    if (nalType == H264_PPS || nalType == H264_SPS || nalType == H264_SEI) {
        _append = true;
    } else {
        _append = false;
    }

    if (_vecFrame.empty()) {
        _msgLength += 9 + length;
    } else {
        _msgLength += 4 + length;
    }
    // logInfo << "is keyframe: " << frame->keyFrame();
    // logInfo << "nal type: " << (int)nalType;
    _vecFrame.push_back(frame);

    // int cts = frame->pts() - frame->dts();
    // msg->payload.reset(new char[9 + length]);
    // auto data = msg->payload.get();
    // if (keyFrame) {
    //     *data++ = 0x17;
    // } else {
    //     *data++ = 0x27;
    // }
    // *data++ = 1;
    // *data++ = (uint8_t)(cts >> 16); 
    // *data++ = (uint8_t)(cts >> 8); 
    // *data++ = (uint8_t)(cts); 

    
    // *data++ = (uint8_t)(length >> 24); 
    // *data++ = (uint8_t)(length >> 16); 
    // *data++ = (uint8_t)(length >> 8); 
    // *data++ = (uint8_t)(length);

    // memcpy(data, frame->data() + frame->startSize(), length);

    // msg->abs_timestamp = frame->pts();
    // msg->trackIndex_ = frame->_index;
    // msg->length = 9 + length;
    // msg->type_id = RTMP_VIDEO;
    // msg->csid = RTMP_CHUNK_VIDEO_ID;

    // onRtmpMessage(msg);

    _lastStamp = frame->pts();
}

void RtmpEncodeH264::setOnRtmpPacket(const function<void(const RtmpMessage::Ptr& msg, bool start)>& cb)
{
    _onRtmpMessage = cb;
}

void RtmpEncodeH264::onRtmpMessage(const RtmpMessage::Ptr& msg, bool start)
{
    if (_onRtmpMessage) {
        _onRtmpMessage(msg, start);
    }
}
