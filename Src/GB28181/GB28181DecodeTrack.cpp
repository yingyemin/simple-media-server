#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "GB28181DecodeTrack.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/Base64.h"
// #include "Codec/AacTrack.h"
// #include "Codec/H264Track.h"
// #include "Codec/H265Track.h"

using namespace std;


GB28181DecodeTrack::GB28181DecodeTrack(int trackIndex)
    :_index(trackIndex)
{
    _trackInfo = make_shared<TrackInfo>();
    _trackInfo->codec_ = "ps";
    _trackInfo->index_ = 0;
    _trackInfo->samplerate_ = 90000;
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
                self->onPsFrame(frame);
            }
        });
    }

    if (!_demuxer) {
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
    if (_onFrame) {
        _onFrame(frame);
    }
    // if (frame->_index == VideoTrackType) {
    //     FILE* fp = fopen("test3.264", "ab+");
    //     fwrite(frame->_buffer.data(), 1, frame->_buffer.size(), fp);
    //     fclose(fp);
    // }
    // logInfo << "decode a frame";
}

void GB28181DecodeTrack::setOnTrackInfo(const function<void(const shared_ptr<TrackInfo>& trackInfo)>& cb)
{
    _onTrackInfo = cb;
}

void GB28181DecodeTrack::setOnReady(const function<void()>& cb)
{
    _onReady = cb;
}

