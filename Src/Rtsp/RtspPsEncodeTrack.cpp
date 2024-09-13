#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtspPsEncodeTrack.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/Base64.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"

using namespace std;

RtspPsEncodeTrack::RtspPsEncodeTrack(int trackIndex)
    :_index(trackIndex)
    ,_type(VideoTrackType)
{
    _trackInfo = make_shared<TrackInfo>();
    _trackInfo->codec_ = "ps";
    _trackInfo->index_ = 0;
    _trackInfo->samplerate_ = 90000;
    _trackInfo->payloadType_ = 96;
    _trackInfo->trackType_ = "video";
    _trackInfo->ssrc_ = _trackInfo->index_;

    _media = make_shared<SdpMedia>();
    _media->payloadType_ = _trackInfo->payloadType_;
    _media->samplerate_ = _trackInfo->samplerate_;
    _media->index_ = _trackInfo->index_;
    _media->channel_ = _trackInfo->channel_;
    _media->trackType_ = _trackInfo->trackType_;
    _media->codec_ = _trackInfo->codec_;
}

void RtspPsEncodeTrack::startEncode()
{
    logInfo << "RtspPsEncodeTrack::startEncode";
    weak_ptr<RtspPsEncodeTrack> wSelf = dynamic_pointer_cast<RtspPsEncodeTrack>(shared_from_this());
    if (!_encoder) {
        _encoder = RtpEncoder::create(_trackInfo);
        _encoder->setOnRtpPacket([wSelf](const RtpPacket::Ptr& rtp, bool start){
            auto self = wSelf.lock();
            self->onRtpPacket(rtp, start);
        });
    }

    if (!_muxer) {
        _muxer = make_shared<PsMuxer>();
        for (auto& it : _mapTrackInfo) {
            _muxer->addTrackInfo(it.second);
        }
        _muxer->setOnPsFrame([wSelf](const FrameBuffer::Ptr &frame){
            auto self = wSelf.lock();
            if (self)
                self->onPsFrame(frame);
        });
        _muxer->startEncode();
    }
}

void RtspPsEncodeTrack::onFrame(const FrameBuffer::Ptr& frame)
{
    if (_muxer) {
        // logInfo << "ps mux a frame";
        if (frame->keyFrame()) {
            if (_mapTrackInfo[frame->_index]->codec_ == "h264") {
                auto trackinfo = dynamic_pointer_cast<H264Track>(_mapTrackInfo[frame->_index]);
                trackinfo->_sps->_dts = frame->_dts;
                trackinfo->_sps->_pts = frame->_pts;
                trackinfo->_sps->_index = frame->_index;
                _muxer->onFrame(trackinfo->_sps);

                trackinfo->_pps->_dts = frame->_dts;
                trackinfo->_pps->_pts = frame->_pts;
                trackinfo->_pps->_index = frame->_index;
                _muxer->onFrame(trackinfo->_pps);
            } else if (_mapTrackInfo[frame->_index]->codec_ == "h265") {
                auto trackinfo = dynamic_pointer_cast<H265Track>(_mapTrackInfo[frame->_index]);
                trackinfo->_vps->_dts = frame->_dts;
                trackinfo->_vps->_pts = frame->_pts;
                trackinfo->_vps->_index = frame->_index;
                _muxer->onFrame(trackinfo->_vps);

                trackinfo->_sps->_dts = frame->_dts;
                trackinfo->_sps->_pts = frame->_pts;
                trackinfo->_sps->_index = frame->_index;
                _muxer->onFrame(trackinfo->_sps);

                trackinfo->_pps->_dts = frame->_dts;
                trackinfo->_pps->_pts = frame->_pts;
                trackinfo->_pps->_index = frame->_index;
                _muxer->onFrame(trackinfo->_pps);
            }
            // FILE* fp = fopen("psmuxer.sps", "ab+");
            // fwrite(trackinfo->_sps->data(), trackinfo->_sps->size(), 1, fp);
            // fclose(fp);
        }
        _muxer->onFrame(frame);
    }
}

void RtspPsEncodeTrack::onPsFrame(const FrameBuffer::Ptr& frame)
{
    if (_encoder) {
        // logInfo << "encode a frame";
        _encoder->encode(frame);
    }
}

void RtspPsEncodeTrack::onRtpPacket(const RtpPacket::Ptr& rtp, bool start)
{
    auto data = rtp->data();
    data[0] = '$';
    data[1] = _index * 2;
    data[2] = (rtp->size() - 4) >> 8;
    data[3] = (rtp->size() - 4) & 0x00FF;
    if (_onRtpPacket) {
        _onRtpPacket(rtp, start);
    }
    // logInfo << "encode a rtp packet";
    _ssrc = rtp->getSSRC();
    _seq = rtp->getSeq();
    _timestap = rtp->getStamp();
}
