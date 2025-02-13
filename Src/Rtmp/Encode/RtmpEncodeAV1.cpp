#include "RtmpEncodeAV1.h"
#include "Codec/AV1Frame.h"
#include "Log/Logger.h"
#include "Codec/AV1Track.h"
#include "Rtmp/Rtmp.h"
#include "Common/Config.h"

using namespace std;

RtmpEncodeAV1::RtmpEncodeAV1(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{}

RtmpEncodeAV1::~RtmpEncodeAV1()
{
}

string RtmpEncodeAV1::getConfig()
{
    auto trackInfo = dynamic_pointer_cast<AV1Track>(_trackInfo);
    if (!trackInfo->_sequence) {
        return "";
    }

    // static bool enbaleEnhanced = Config::instance()->getAndListen([](const json &config){
    //     enbaleEnhanced = Config::instance()->get("Rtmp", "Server", "Server1", "enableEnhanced");
    // }, "Rtmp", "Server", "Server1", "enableEnhanced");

    string config;

    auto vpsBuffer = trackInfo->_sequence->_buffer;
    int vpsLen = trackInfo->_sequence->size();

    config.resize(9  + vpsLen);
    auto data = (char*)config.data();

    if (_enhanced) {
        *data++ = 1 << 7 | 1  << 4;
        *data++ = 'a';
        *data++ = 'v';
        *data++ = '0';
        *data++ = '1';
    } else {
        *data++ = 0x1c; //key frame, AVC
        *data++ = 0x00; //avc sequence header
        *data++ = 0x00; //composit time 
        *data++ = 0x00; //composit time
        *data++ = 0x00; //composit time
    }

    *data++ = 1 << 7 | 1;
    *data++ = 0xff;
    *data++ = 0xff;
    *data++ = 0xff;

    memcpy(data, vpsBuffer.data(), vpsLen); 

    // TODO meta data frame

    return config;
}

void RtmpEncodeAV1::encode(const FrameBuffer::Ptr& frame)
{
    // static bool enbaleEnhanced = Config::instance()->getAndListen([](const json &config){
    //     enbaleEnhanced = Config::instance()->get("Rtmp", "Server", "Server1", "enableEnhanced");
    // }, "Rtmp", "Server", "Server1", "enableEnhanced");

    int index = 0;
    bool start = false;
    auto msg = make_shared<RtmpMessage>();
    msg->payload = make_shared<StreamBuffer>(8 + frame->size() + 1); // max size
    auto data = msg->payload->data();
    auto keyFrame = frame->keyFrame() || frame->metaFrame();
    int cts = frame->pts() - frame->dts();
    int length = frame->size();
    
    if (frame->startFrame()) {
        start = true;
    }

    if (_enhanced) {
        uint8_t frameType = keyFrame ? 1 : 2;
        *data++ = 1 << 7 | frameType << 4 | 1;
        *data++ = 'a';
        *data++ = 'v';
        *data++ = '0';
        *data++ = '1';
        // *data++ = (uint8_t)(cts >> 16); 
        // *data++ = (uint8_t)(cts >> 8); 
        // *data++ = (uint8_t)(cts); 
        index += 5;
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
    // max 8

    memcpy(data, frame->data(), length);

    msg->payload->setSize(index + length);
    msg->abs_timestamp = _lastStamp;
    msg->trackIndex_ = _trackInfo->index_;
    msg->length = index + length;
    msg->type_id = RTMP_VIDEO;
    msg->csid = RTMP_CHUNK_VIDEO_ID;

    onRtmpMessage(msg, start);

    _lastStamp = frame->pts();
}

void RtmpEncodeAV1::setOnRtmpPacket(const function<void(const RtmpMessage::Ptr& msg, bool start)>& cb)
{
    _onRtmpMessage = cb;
}

void RtmpEncodeAV1::onRtmpMessage(const RtmpMessage::Ptr& msg, bool start)
{
    if (_onRtmpMessage) {
        _onRtmpMessage(msg, start);
    }
}
