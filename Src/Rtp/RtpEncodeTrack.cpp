#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpEncodeTrack.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "Util/Base64.h"
// #include "Codec/AacTrack.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"

using namespace std;

RtpEncodeTrack::RtpEncodeTrack(int trackIndex, const string& payloadType)
    :_index(trackIndex)
    ,_payloadType(payloadType)
{
    logInfo << "RtpEncodeTrack::RtpEncodeTrack";
    if (_payloadType == "ts" || _payloadType == "ps") {
        _trackInfo = make_shared<TrackInfo>();
        _trackInfo->codec_ = _payloadType;
        _trackInfo->index_ = 0;
        _trackInfo->samplerate_ = 90000;
        _trackInfo->ssrc_ = _trackInfo->index_;
        _trackInfo->payloadType_ = 96;
    }
}

void RtpEncodeTrack::startEncode()
{
    weak_ptr<RtpEncodeTrack> wSelf = dynamic_pointer_cast<RtpEncodeTrack>(shared_from_this());
    if (!_encoder) {
        _encoder = RtpEncoder::create(_trackInfo);
        _encoder->setSsrc(_ssrc);
        _encoder->setOnRtpPacket([wSelf](const RtpPacket::Ptr& rtp, bool start){
            auto self = wSelf.lock();
            if (self)
                self->onRtpPacket(rtp);
        });
    }
    if (!_psMuxer && _payloadType == "ps") {
        _psMuxer = make_shared<PsMuxer>();
        for (auto& it : _mapTrackInfo) {
            _psMuxer->addTrackInfo(it.second);
        }
        _psMuxer->setOnPsFrame([wSelf](const FrameBuffer::Ptr &frame){
            auto self = wSelf.lock();
            if (self)
                self->onPsFrame(frame);
        });
        _psMuxer->startEncode();
    }
    if (!_tsMuxer && _payloadType == "ts") {
        _tsMuxer = make_shared<TsMuxer>();
        for (auto& it : _mapTrackInfo) {
            _tsMuxer->addTrackInfo(it.second);
        }
        _tsMuxer->setOnTsPacket([wSelf](const StreamBuffer::Ptr &buffer, int pts, int dts, bool keyframe){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            auto frame = make_shared<FrameBuffer>();
            frame->_buffer->assign(buffer->data(), buffer->size());
            frame->_pts = pts;
            frame->_dts = dts;
            frame->_trackType = self->_trackInfo->trackType_ == "video" ? VideoTrackType : AudioTrackType;
            frame->_index = self->_trackInfo->index_;
            frame->_codec = self->_trackInfo->codec_;
            self->onTsFrame(frame);
        });
        _tsMuxer->startEncode();
    }
}

void RtpEncodeTrack::onFrame(const FrameBuffer::Ptr& frame)
{
    if (_psMuxer && _payloadType == "ps") {
        // logInfo << "ps mux a frame";
        _psMuxer->onFrame(frame);
    } else if (_tsMuxer && _payloadType == "ts") {
        // logInfo << "ts mux a frame";
        _tsMuxer->onFrame(frame);
    } else {
        _encoder->encode(frame);
    }
}

void RtpEncodeTrack::onPsFrame(const FrameBuffer::Ptr& frame)
{
    if (_encoder) {
        logTrace << "rtp encode a ps frame";
        _encoder->encode(frame);
    }
    // FILE* fp = fopen("psmuxer.ps", "ab+");
    // fwrite(frame->data(), frame->size(), 1, fp);
    // fclose(fp);
}

void RtpEncodeTrack::onTsFrame(const FrameBuffer::Ptr& frame)
{
    if (_encoder) {
        logTrace << "rtp encode a ps frame";
        _encoder->encode(frame);
    }
    // FILE* fp = fopen("psmuxer.ps", "ab+");
    // fwrite(frame->data(), frame->size(), 1, fp);
    // fclose(fp);
}

void RtpEncodeTrack::onRtpPacket(const RtpPacket::Ptr& rtp)
{
    // 国标的rtp over tcp前面只要留两个字节
    rtp->buffer()->substr(2);

    auto data = rtp->data();
    data[0] = (rtp->size() - 4) >> 8;
    data[1] = (rtp->size() - 4) & 0x00FF;

    if (_onRtpPacket) {
        _onRtpPacket(rtp);
    }
    logTrace << "encode a rtp packet";
    // _ssrc = rtp->getSSRC();
    // _seq = rtp->getSeq();
    // _timestap = rtp->getStamp();
}
