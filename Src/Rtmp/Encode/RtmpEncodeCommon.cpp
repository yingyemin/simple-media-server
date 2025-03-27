#include "RtmpEncodeCommon.h"
#include "Logger.h"
#include "Rtmp/Rtmp.h"
#include "Codec/AacTrack.h"

using namespace std;

int getFalg(const shared_ptr<TrackInfo>& trackInfo)
{
    int audioType;
    if (trackInfo->codec_ == "g711a") {
        audioType = RTMP_CODEC_ID_G711A;
    } else if (trackInfo->codec_ == "g711u") {
        audioType = RTMP_CODEC_ID_G711U;
    } else if (trackInfo->codec_ == "mp3") {
        audioType = RTMP_CODEC_ID_MP3;
    } else if (trackInfo->codec_ == "adpcma") {
        audioType = RTMP_CODEC_ID_ADPCM;
    } else {
        return 0;
    }

    int sampleRate = trackInfo->samplerate_;
    int sampleBit = trackInfo->bitPerSample_;
    int channel = trackInfo->channel_;

    uint8_t flvSampleRate;
    switch (sampleRate) {
        case 44100:
            flvSampleRate = 3;
            break;
        case 22050:
            flvSampleRate = 2;
            break;
        case 11025:
            flvSampleRate = 1;
            break;
        case 16000: // nellymoser only
        case 8000: // nellymoser only
        case 5512: // not MP3
            flvSampleRate = 0;
            break;
        default:
            logWarn << "FLV does not support sample rate " << sampleRate << " ,choose from (44100, 22050, 11025)";
            return 0;
    }

    uint8_t flvStereoOrMono = (channel > 1);
    uint8_t flvSampleBit = sampleBit == 16;

    return (audioType << 4) | (flvSampleRate << 2) | (flvSampleBit << 1) | flvStereoOrMono;
}

RtmpEncodeCommon::RtmpEncodeCommon(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{
    _audioFlag = getFalg(_trackInfo);
}

RtmpEncodeCommon::~RtmpEncodeCommon()
{
}

string RtmpEncodeCommon::getConfig()
{
    return "";
}

void RtmpEncodeCommon::encode(const FrameBuffer::Ptr& frame)
{
    int length = frame->size() - frame->startSize();

    auto msg = make_shared<RtmpMessage>();
    msg->payload = make_shared<StreamBuffer>(1 + length + 1);
    auto data = msg->payload->data();
    
    *data++ = _audioFlag;
    memcpy(data, frame->data() + frame->startSize(), length);

    msg->abs_timestamp = frame->pts();
    msg->trackIndex_ = frame->_index;
    msg->length = 1 + length;
    msg->type_id = RTMP_AUDIO;
    msg->csid = RTMP_CHUNK_AUDIO_ID;

    onRtmpMessage(msg, frame->startFrame());
}

void RtmpEncodeCommon::setOnRtmpPacket(const function<void(const RtmpMessage::Ptr& msg, bool start)>& cb)
{
    _onRtmpMessage = cb;
}

void RtmpEncodeCommon::onRtmpMessage(const RtmpMessage::Ptr& msg, bool start)
{
    if (_onRtmpMessage) {
        _onRtmpMessage(msg, start);
    }
}
