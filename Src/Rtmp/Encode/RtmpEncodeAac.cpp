#include "RtmpEncodeAac.h"
#include "Logger.h"
#include "Rtmp/Rtmp.h"
#include "Codec/AacTrack.h"

using namespace std;

int getAacFalg(const shared_ptr<TrackInfo>& trackInfo)
{
    int audioType = RTMP_CODEC_ID_AAC;
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

RtmpEncodeAac::RtmpEncodeAac(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{}

RtmpEncodeAac::~RtmpEncodeAac()
{
}

string RtmpEncodeAac::getConfig()
{
    string config;

    auto aacTrack = dynamic_pointer_cast<AacTrack>(_trackInfo);

    auto aacConfig = aacTrack->getAacInfo();
    int length = aacConfig.size();

    config.resize(2 + length);
    auto data = (char*)config.data();

    _audioFlag = getAacFalg(_trackInfo);
    *data++ = _audioFlag;
    *data++ = 0;
    memcpy(data, aacConfig.data(), length);

    return config;
}

void RtmpEncodeAac::encode(const FrameBuffer::Ptr& frame)
{
    int length = frame->size() - frame->startSize();

    auto msg = make_shared<RtmpMessage>();
    msg->payload = make_shared<StreamBuffer>(2 + length + 1);
    auto data = msg->payload->data();
    
    *data++ = _audioFlag;
    *data++ = 1;
    memcpy(data, frame->data() + frame->startSize(), length);

    msg->abs_timestamp = frame->pts();
    msg->trackIndex_ = frame->_index;
    msg->length = 2 + length;
    msg->type_id = RTMP_AUDIO;
    msg->csid = RTMP_CHUNK_AUDIO_ID;

    onRtmpMessage(msg, frame->startFrame());
}

void RtmpEncodeAac::setOnRtmpPacket(const function<void(const RtmpMessage::Ptr& msg, bool start)>& cb)
{
    _onRtmpMessage = cb;
}

void RtmpEncodeAac::onRtmpMessage(const RtmpMessage::Ptr& msg, bool start)
{
    if (_onRtmpMessage) {
        _onRtmpMessage(msg, start);
    }
}
