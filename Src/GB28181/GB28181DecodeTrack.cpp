#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "GB28181DecodeTrack.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/Base64.h"
#include "Codec/AacTrack.h"
#include "Codec/G711Track.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"
#include "Codec/H264Frame.h"
#include "Codec/H265Frame.h"

using namespace std;


GB28181DecodeTrack::GB28181DecodeTrack(int trackIndex)
    :_index(trackIndex)
{
    _trackInfo = make_shared<TrackInfo>();
    _trackInfo->codec_ = "ps";
    _trackInfo->index_ = 0;
    _trackInfo->samplerate_ = 90000;

    _isPs = true;
}

GB28181DecodeTrack::GB28181DecodeTrack(int trackIndex, bool isPs)
    :_index(trackIndex)
    ,_isPs(isPs)
{

}

void GB28181DecodeTrack::startDecode()
{
    weak_ptr<GB28181DecodeTrack> wSelf = shared_from_this();
    if (!_decoder) {
        _decoder = RtpDecoder::creatDecoder(_trackInfo);
        if (!_decoder) {
            logInfo << "不支持的编码：" << _trackInfo->codec_;
            throw "不支持的编码";
        }
        _decoder->setOnDecode([wSelf](const FrameBuffer::Ptr& frame){
            auto self = wSelf.lock();
            if (self) {
                if (self->_isPs) {
                    self->onPsFrame(frame);
                } else {
                    self->onFrame(frame);
                }
            }
        });
    }

    if (!_demuxer && _isPs) {
        _demuxer = make_shared<PsDemuxer>();
        _demuxer->setOnDecode([wSelf](const FrameBuffer::Ptr &frame){
            auto self = wSelf.lock();
            if (self) {
                self->onFrame(frame);
            }
        });

        _demuxer->setOnTrackInfo([wSelf](const shared_ptr<TrackInfo>& trackInfo){
            auto self = wSelf.lock();
            if (self && self->_onTrackInfo) {
                self->_onTrackInfo(trackInfo);
            }
        });

        _demuxer->setOnReady([wSelf](){
            auto self = wSelf.lock();
            if (self && self->_onReady) {
                self->_onReady();
            }
        });
    }
    _startdecode = true;
    _startDemuxer = true;
}

void GB28181DecodeTrack::stopDecode()
{
    // _decoder = nullptr;
    // _demuxer = nullptr;
    _startDemuxer = false;
    _startdecode = false;
}

void GB28181DecodeTrack::onRtpPacket(const RtpPacket::Ptr& rtp)
{
    if (_onRtpPacket) {
        _onRtpPacket(rtp);
    }
    // logInfo << "encode a rtp packet" << rtp->getSeq();
    _ssrc = rtp->getSSRC();
    _seq = rtp->getSeq();
    _timestap = rtp->getStamp();
}

void GB28181DecodeTrack::decodeRtp(const RtpPacket::Ptr& rtp)
{
    // logInfo << "rtp seq: " << rtp->getSeq();
    // logInfo << "track info codec: " << _trackInfo->codec_;
    if (!_startdecode) {
        _decoder = nullptr;
        return;
    }
    if (_decoder) {
        _decoder->decode(rtp);
    }
}

void GB28181DecodeTrack::onPsFrame(const FrameBuffer::Ptr frame)
{
    // logInfo << "get a ps frame: " << frame->size();
    if (!_startDemuxer) {
        _demuxer = nullptr;
        return ;
    }
    if (_onPsFrame) {
        _onPsFrame(frame);
    }
    if (_demuxer) {
        _demuxer->onPsStream(frame->data(), frame->size(), frame->pts(), _ssrc);
    }
    // FILE* fp = fopen("test.ps", "ab+");
    // fwrite(frame->_buffer.data(), 1, frame->_buffer.size(), fp);
    // fclose(fp);
}

void GB28181DecodeTrack::onFrame(const FrameBuffer::Ptr& frame)
{
    // logInfo << "get a raw frame: " << frame->_codec;
    if (!_isPs) {
        if (_trackInfo->codec_ == "h265") {
            auto h265frame = dynamic_pointer_cast<H265Frame>(frame);
            h265frame->split([this, h265frame](const FrameBuffer::Ptr &subFrame){
                if (_firstVps || _firstSps || _firstPps) {
                    auto h265Track = dynamic_pointer_cast<H265Track>(_trackInfo);
                    auto nalType = subFrame->getNalType();
                    switch (nalType)
                    {
                    case H265_VPS:
                        h265Track->setVps(subFrame);
                        _firstVps = false;
                        break;
                    case H265_SPS:
                        h265Track->setSps(subFrame);
                        _firstSps = false;
                        break;
                    case H265_PPS:
                        h265Track->setPps(subFrame);
                        _firstPps = false;
                        break;
                    default:
                        break;
                    }
                }else {
                    logInfo << "gb28181 _onReady";
                    if (!_hasReady && _onReady) {
                        _onReady();
                        _hasReady = true;
                    }
                }
                if (_onFrame) {
                    _onFrame(subFrame);
                }
            });
        } else if (_trackInfo->codec_ == "h264") {
            auto h264frame = dynamic_pointer_cast<H264Frame>(frame);
            h264frame->split([this, h264frame](const FrameBuffer::Ptr &subFrame){
                if (_firstSps || _firstPps) {
                    auto h264Track = dynamic_pointer_cast<H264Track>(_trackInfo);
                    auto nalType = subFrame->getNalType();
                    logInfo << "get a h264 nal type: " << (int)nalType;
                    switch (nalType)
                    {
                    case H264_SPS:
                        h264Track->setSps(subFrame);
                        _firstSps = false;
                        break;
                    case H264_PPS:
                        h264Track->setPps(subFrame);
                        _firstPps = false;
                        break;
                    default:
                        break;
                    }
                } else {
                    // logInfo << "gb28181 _onReady";
                    if (!_hasReady && _onReady) {
                        _onReady();
                        _hasReady = true;
                    }
                }
                if (_onFrame) {
                    _onFrame(subFrame);
                }
            });
        }
    } else {
        if (_onFrame) {
            _onFrame(frame);
        }
    }
    // if (frame->_index == VideoTrackType) {
        // FILE* fp = fopen("test3.aac", "ab+");
        // fwrite(frame->_buffer.data(), 1, frame->_buffer.size(), fp);
        // fclose(fp);
    // }
    // logInfo << "decode a frame: " << frame->_index << ", decoder codec: " << _trackInfo->codec_;
}

void GB28181DecodeTrack::setOnTrackInfo(const function<void(const shared_ptr<TrackInfo>& trackInfo)>& cb)
{
    _onTrackInfo = cb;
}

void GB28181DecodeTrack::setOnReady(const function<void()>& cb)
{
    _onReady = cb;
}

void GB28181DecodeTrack::createVideoTrack(const string& videoCodec)
{
    // TrackInfo::Ptr track;
    if (videoCodec == "h264") {
        _trackInfo = H264Track::createTrack(0, 96, 90000);
    } else if (videoCodec == "h265") {
        _trackInfo = make_shared<H265Track>();
    }
    _trackInfo->codec_ = videoCodec;
    _trackInfo->index_ = 0;
    _trackInfo->samplerate_ = 90000;
    _trackInfo->payloadType_ = 96;

    // if (_onTrackInfo) {
    //     _onTrackInfo(track);
    // }
}

void GB28181DecodeTrack::createAudioTrack(const string& audioCodec, int channel, int sampleBit, int sampleRate)
{
    // TrackInfo::Ptr track;
    if (audioCodec == "g711a") {
        _trackInfo = make_shared<G711aTrack>();
        _trackInfo->payloadType_ = 8;
    } else if (audioCodec == "g711u") {
        _trackInfo = make_shared<G711uTrack>();
        _trackInfo->payloadType_ = 0;
    } else if (audioCodec == "aac") {
        AacTrack::Ptr track = make_shared<AacTrack>();
        track->codec_ = audioCodec;
        track->index_ = 1;
        track->samplerate_ = sampleRate;
        track->channel_ = channel;
        track->bitPerSample_ = sampleBit;
        track->payloadType_ = 104;

        track->setAacInfo(2, channel, sampleRate);
        _trackInfo = track;
        return ;
    }
    _trackInfo->codec_ = audioCodec;
    _trackInfo->index_ = 1;
    _trackInfo->samplerate_ = sampleRate;
    _trackInfo->channel_ = channel;
    _trackInfo->bitPerSample_ = sampleBit;

    // if (_onTrackInfo) {
    //     _onTrackInfo(track);
    // }
}
