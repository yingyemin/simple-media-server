#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpDecodeTrack.h"
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

RtpDecodeTrack::RtpDecodeTrack(int trackIndex)
    :_index(trackIndex)
{
    _trackInfo = make_shared<TrackInfo>();
    _trackInfo->codec_ = "ps";
    _trackInfo->index_ = 0;
    _trackInfo->samplerate_ = 90000;

    _payloadType = "ps";
}

RtpDecodeTrack::RtpDecodeTrack(int trackIndex, const string& payloadType)
    :_index(trackIndex)
    ,_payloadType(payloadType)
{
    if (payloadType == "ps" || payloadType == "ts") {
        _trackInfo = make_shared<TrackInfo>();
        _trackInfo->codec_ = payloadType;
        _trackInfo->index_ = 0;
        _trackInfo->samplerate_ = 90000;
    }
}

void RtpDecodeTrack::startDecode()
{
    if (!_trackInfo) {
        logWarn << "no track info";
        return ;
    }
    weak_ptr<RtpDecodeTrack> wSelf = shared_from_this();
    if (!_decoder) {
        _decoder = RtpDecoder::creatDecoder(_trackInfo);
        if (!_decoder) {
            logInfo << "不支持的编码：" << _trackInfo->codec_;
            throw "不支持的编码";
        }
        _decoder->setOnDecode([wSelf](const FrameBuffer::Ptr& frame){
            auto self = wSelf.lock();
            if (self) {
                if (self->_payloadType == "ps") {
                    self->onPsFrame(frame);
                } else if (self->_payloadType == "ts") {
                    self->onTsFrame(frame);
                } else {
                    self->onFrame(frame);
                }
            }
        });
    }

    if (!_psDemuxer && _payloadType == "ps") {
        _psDemuxer = make_shared<PsDemuxer>();
        _psDemuxer->setOnDecode([wSelf](const FrameBuffer::Ptr &frame){
            auto self = wSelf.lock();
            if (self) {
                self->onFrame(frame);
            }
        });

        _psDemuxer->setOnTrackInfo([wSelf](const shared_ptr<TrackInfo>& trackInfo){
            auto self = wSelf.lock();
            if (self && self->_onTrackInfo) {
                self->_onTrackInfo(trackInfo);
            }
        });

        _psDemuxer->setOnReady([wSelf](){
            auto self = wSelf.lock();
            if (self && self->_onReady) {
                self->_onReady();
            }
        });
    } else if (!_tsDemuxer && _payloadType == "ts") {
        _tsDemuxer = make_shared<TsDemuxer>();
        _tsDemuxer->setOnDecode([wSelf](const FrameBuffer::Ptr &frame){
            auto self = wSelf.lock();
            if (self) {
                self->onFrame(frame);
            }
        });

        _tsDemuxer->setOnTrackInfo([wSelf](const shared_ptr<TrackInfo>& trackInfo){
            auto self = wSelf.lock();
            if (self && self->_onTrackInfo) {
                self->_onTrackInfo(trackInfo);
            }
        });

        _tsDemuxer->setOnReady([wSelf](){
            auto self = wSelf.lock();
            if (self && self->_onReady) {
                self->_onReady();
            }
        });
    }

    _startdecode = true;
    _startDemuxer = true;
}

void RtpDecodeTrack::stopDecode()
{
    // _decoder = nullptr;
    // _demuxer = nullptr;
    _startDemuxer = false;
    _startdecode = false;
}

void RtpDecodeTrack::onRtpPacket(const RtpPacket::Ptr& rtp)
{
    if (_onRtpPacket) {
        _onRtpPacket(rtp);
    }
    // logInfo << "encode a rtp packet" << rtp->getSeq();
    // _ssrc = rtp->getSSRC();
    // _seq = rtp->getSeq();
    // _timestap = rtp->getStamp();
}

void RtpDecodeTrack::decodeRtp(const RtpPacket::Ptr& rtp)
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

void RtpDecodeTrack::onPsFrame(const FrameBuffer::Ptr frame)
{
    // logInfo << "get a ps frame: " << frame->size();
    if (!_startDemuxer) {
        _psDemuxer = nullptr;
        return ;
    }
    if (_onPsFrame) {
        _onPsFrame(frame);
    }
    if (_psDemuxer) {
        _psDemuxer->onPsStream(frame->data(), frame->size(), frame->pts(), _ssrc, true);
    }
    // FILE* fp = fopen("test.ps", "ab+");
    // fwrite(frame->_buffer.data(), 1, frame->_buffer.size(), fp);
    // fclose(fp);
}

void RtpDecodeTrack::onTsFrame(const FrameBuffer::Ptr frame)
{
    // logInfo << "get a ps frame: " << frame->size();
    if (!_startDemuxer) {
        _tsDemuxer = nullptr;
        return ;
    }
    if (_onPsFrame) {
        _onPsFrame(frame);
    }
    if (_tsDemuxer) {
        _tsDemuxer->onTsPacket(frame->data(), frame->size(), frame->pts());
    }
    // FILE* fp = fopen("test.ps", "ab+");
    // fwrite(frame->_buffer.data(), 1, frame->_buffer.size(), fp);
    // fclose(fp);
}

void RtpDecodeTrack::onFrame(const FrameBuffer::Ptr& frame)
{
    logInfo << "get a raw frame: " << frame->_codec << ", type: " << (int)frame->getNalType();
    if (_payloadType == "ts" || _payloadType == "ps") {
        if (_onFrame) {
            _onFrame(frame);
        }
    } else {
        if (_trackInfo->codec_ == "h265") {
            auto h265frame = dynamic_pointer_cast<H265Frame>(frame);
            h265frame->split([this, h265frame](const FrameBuffer::Ptr &subFrame){
                if (_onFrame) {
                    _onFrame(subFrame);
                }
            });
        } else if (_trackInfo->codec_ == "h264") {
            auto h264frame = dynamic_pointer_cast<H264Frame>(frame);
            h264frame->split([this, h264frame](const FrameBuffer::Ptr &subFrame){
                if (_onFrame) {
                    _onFrame(subFrame);
                }
            });
        } else {
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

void RtpDecodeTrack::setOnTrackInfo(const function<void(const shared_ptr<TrackInfo>& trackInfo)>& cb)
{
    _onTrackInfo = cb;
}

void RtpDecodeTrack::setOnReady(const function<void()>& cb)
{
    _onReady = cb;
}

void RtpDecodeTrack::createVideoTrack(const string& videoCodec)
{
    // TrackInfo::Ptr track;
    if (videoCodec == "h264") {
        _trackInfo = H264Track::createTrack(0, 96, 90000);
    } else if (videoCodec == "h265") {
        _trackInfo = make_shared<H265Track>();
    } else {
        logInfo << "invalid video codec : " << videoCodec;
        return ;
    }
    _trackInfo->codec_ = videoCodec;
    _trackInfo->index_ = 0;
    _trackInfo->samplerate_ = 90000;
    _trackInfo->payloadType_ = 96;

    // if (_onTrackInfo) {
    //     _onTrackInfo(track);
    // }
}

void RtpDecodeTrack::createAudioTrack(const string& audioCodec, int channel, int sampleBit, int sampleRate)
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
        track->payloadType_ = 97;

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
