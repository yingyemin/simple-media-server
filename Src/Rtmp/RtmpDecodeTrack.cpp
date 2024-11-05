#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtmpDecodeTrack.h"
#include "Util/String.h"
#include "Util/Base64.h"
#include "Log/Logger.h"
#include "Codec/AacTrack.h"
#include "Codec/G711Track.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"
#include "Decode/RtmpDecodeAac.h"
#include "Decode/RtmpDecodeCommon.h"
#include "Decode/RtmpDecodeH264.h"
#include "Decode/RtmpDecodeH265.h"
#include "Rtmp.h"

using namespace std;


RtmpDecodeTrack::RtmpDecodeTrack(int trackIndex)
    :_index(trackIndex)
{
}

void RtmpDecodeTrack::createDecoder()
{
    weak_ptr<RtmpDecodeTrack> wSelf = shared_from_this();
    if (!_decoder) {
        if (_trackInfo->codec_ == "h264") {
            _decoder = dynamic_pointer_cast<RtmpDecode>(make_shared<RtmpDecodeH264>(_trackInfo));
        } else if (_trackInfo->codec_ == "h265") {
            _decoder = dynamic_pointer_cast<RtmpDecode>(make_shared<RtmpDecodeH265>(_trackInfo));
        } else if (_trackInfo->codec_ == "aac") {
            _decoder = dynamic_pointer_cast<RtmpDecode>(make_shared<RtmpDecodeAac>(_trackInfo));
        } else if (_trackInfo->codec_ == "g711a" || _trackInfo->codec_ == "g711u") {
            _decoder = dynamic_pointer_cast<RtmpDecode>(make_shared<RtmpDecodeCommon>(_trackInfo));
        }

        if (!_decoder) {
            logInfo << "不支持的编码：" << _trackInfo->codec_;
            throw "不支持的编码";
        }

        _decoder->setOnFrame([wSelf](const FrameBuffer::Ptr& frame){
            auto self = wSelf.lock();
            if (self) {
                self->onFrame(frame);
            }
        });
    }
}

void RtmpDecodeTrack::startDecode()
{
    createDecoder();
}

void RtmpDecodeTrack::stopDecode()
{
    _decoder = nullptr;
}

void RtmpDecodeTrack::setConfigFrame(const RtmpMessage::Ptr& pkt)
{
    if (_decoder) {
        _decoder->decode(pkt);
    } else {
        createDecoder();
        _decoder->decode(pkt);
        _decoder = nullptr;
    }
}

int RtmpDecodeTrack::createTrackInfo(int trackType, int codeId)
{
    if (trackType == VideoTrackType) {
        if (codeId == RTMP_CODEC_ID_H264) {
            _trackInfo = H264Track::createTrack(VideoTrackType, 96, 90000);
        } else if (codeId == RTMP_CODEC_ID_H265) {
            _trackInfo = make_shared<H265Track>();
            _trackInfo->codec_ = "h265";
            _trackInfo->index_ = VideoTrackType;
            _trackInfo->trackType_ = "video";
            _trackInfo->payloadType_ = 96;
            _trackInfo->samplerate_ = 90000;
        } else {
            // throw runtime_error("不支持的解码格式:" + to_string(codeId));
            logWarn << "不支持的video解码格式:" << codeId;
            return -1;
        }
    } else if (trackType == AudioTrackType) {
        if (codeId == RTMP_CODEC_ID_AAC) {
            _trackInfo = make_shared<AacTrack>();
            _trackInfo->codec_ = "aac";
            _trackInfo->index_ = AudioTrackType;
            _trackInfo->trackType_ = "audio";
            _trackInfo->payloadType_ = 97;
            // 此处只是默认值，需要解析aac_config获取真实的
            _trackInfo->samplerate_ = 44100;
            // 此处只是默认值，需要解析aac_config获取真实的
            _trackInfo->channel_ = 2;
        } else if (codeId == RTMP_CODEC_ID_G711A) {
            _trackInfo = make_shared<G711aTrack>();
            _trackInfo->codec_ = "g711a";
            _trackInfo->index_ = AudioTrackType;
            _trackInfo->trackType_ = "audio";
        } else if (codeId == RTMP_CODEC_ID_G711U) {
            _trackInfo = make_shared<G711uTrack>();
            _trackInfo->codec_ = "g711u";
            _trackInfo->index_ = AudioTrackType;
            _trackInfo->trackType_ = "audio";
        } else {
            // throw runtime_error("不支持的解码格式:" + to_string(codeId));
            logWarn << "不支持的audio解码格式:" << codeId;
            return -1;
        }
    }

    return 0;
}

void RtmpDecodeTrack::onRtmpPacket(const RtmpMessage::Ptr& pkt)
{
    if (_onRtmpPacket) {
        _onRtmpPacket(pkt);
    }
    _timestap = pkt->abs_timestamp;
}

void RtmpDecodeTrack::decodeRtmp(const RtmpMessage::Ptr& pkt)
{
    if (_decoder) {
        _decoder->decode(pkt);
    }
}

void RtmpDecodeTrack::onFrame(const FrameBuffer::Ptr& frame)
{
    // logInfo << "get a raw frame: " << frame->_codec;
    if (_onFrame) {
        _onFrame(frame);
    }
    // if (frame->_index == VideoTrackType) {
    //     FILE* fp = fopen("rtmpdecode.264", "ab+");
    //     fwrite(frame->_buffer.data(), 1, frame->_buffer.size(), fp);
    //     fclose(fp);
    // }
    // logInfo << "decode a frame";
}

void RtmpDecodeTrack::setOnTrackInfo(const function<void(const shared_ptr<TrackInfo>& trackInfo)>& cb)
{
    _onTrackInfo = cb;
}

void RtmpDecodeTrack::setOnReady(const function<void()>& cb)
{
    _onReady = cb;
}

