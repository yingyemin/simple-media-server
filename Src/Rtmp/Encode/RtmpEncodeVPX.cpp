#include "RtmpEncodeVPX.h"
#include "Codec/VP8Frame.h"
#include "Codec/VP9Frame.h"
#include "Log/Logger.h"
#include "Codec/VP8Track.h"
#include "Codec/VP9Track.h"
#include "Rtmp/Rtmp.h"
#include "Common/Config.h"

using namespace std;

RtmpEncodeVPX::RtmpEncodeVPX(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{
    _enhanced = true;
}

RtmpEncodeVPX::~RtmpEncodeVPX()
{
}

string RtmpEncodeVPX::getConfig()
{
    // static bool enbaleEnhanced = Config::instance()->getAndListen([](const json &config){
    //     enbaleEnhanced = Config::instance()->get("Rtmp", "Server", "Server1", "enableEnhanced");
    // }, "Rtmp", "Server", "Server1", "enableEnhanced");

    string config;

    string vpxConfig = _trackInfo->getConfig();

    config.resize(5  + vpxConfig.size());
    auto data = (char*)config.data();

    if (_enhanced) {
        *data++ = 1 << 7 | 1  << 4;
        *data++ = 'v';
        *data++ = 'p';
        *data++ = '0';
        *data++ = '9';
        // ffmpeg推的流，后面还有四个字节，不加也可以播
        // *data++ = 0x01;
        // *data++ = 0x00;
        // *data++ = 0x00;
        // *data++ = 0x00;
    } else {
        *data++ = 0x1c; //key frame, AVC
        *data++ = 0x00; //avc sequence header
        *data++ = 0x00; //composit time 
        *data++ = 0x00; //composit time
        *data++ = 0x00; //composit time
    }

    memcpy(data, vpxConfig.data(), vpxConfig.size()); 

    // TODO meta data frame

    return config;
}

void RtmpEncodeVPX::encode(const FrameBuffer::Ptr& frame)
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
        *data++ = 'v';
        *data++ = 'p';
        *data++ = '0';
        *data++ = '9';
        // ffmpeg 没有cts字段，这里先注释
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

void RtmpEncodeVPX::setOnRtmpPacket(const function<void(const RtmpMessage::Ptr& msg, bool start)>& cb)
{
    _onRtmpMessage = cb;
}

void RtmpEncodeVPX::onRtmpMessage(const RtmpMessage::Ptr& msg, bool start)
{
    if (_onRtmpMessage) {
        _onRtmpMessage(msg, start);
    }
}
