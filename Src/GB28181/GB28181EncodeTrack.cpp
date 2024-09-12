#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "GB28181EncodeTrack.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/Base64.h"
// #include "Codec/AacTrack.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"

using namespace std;

GB28181EncodeTrack::GB28181EncodeTrack(int trackIndex)
    :_index(trackIndex)
{
    logInfo << "GB28181EncodeTrack::GB28181EncodeTrack";
    _trackInfo = make_shared<TrackInfo>();
    _trackInfo->codec_ = "ps";
    _trackInfo->index_ = 0;
    _trackInfo->samplerate_ = 90000;
    _trackInfo->ssrc_ = _trackInfo->index_;
}

void GB28181EncodeTrack::startEncode()
{
    weak_ptr<GB28181EncodeTrack> wSelf = dynamic_pointer_cast<GB28181EncodeTrack>(shared_from_this());
    if (!_encoder) {
        _encoder = RtpEncoder::create(_trackInfo);
        _encoder->setOnRtpPacket([wSelf](const RtpPacket::Ptr& rtp, bool start){
            auto self = wSelf.lock();
            if (self)
                self->onRtpPacket(rtp);
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

void GB28181EncodeTrack::onFrame(const FrameBuffer::Ptr& frame)
{
    if (_muxer) {
        logInfo << "ps mux a frame";
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

void GB28181EncodeTrack::onPsFrame(const FrameBuffer::Ptr& frame)
{
    if (_encoder) {
        logInfo << "rtp encode a ps frame";
        _encoder->encode(frame);
    }
    // FILE* fp = fopen("psmuxer.ps", "ab+");
    // fwrite(frame->data(), frame->size(), 1, fp);
    // fclose(fp);
}

void GB28181EncodeTrack::onRtpPacket(const RtpPacket::Ptr& rtp)
{
    // 国标的rtp over tcp前面只要留两个字节
    rtp->buffer()->substr(2);

    auto data = rtp->data();
    data[0] = (rtp->size() - 4) >> 8;
    data[1] = (rtp->size() - 4) & 0x00FF;

    if (_onRtpPacket) {
        _onRtpPacket(rtp);
    }
    logInfo << "encode a rtp packet";
    // _ssrc = rtp->getSSRC();
    // _seq = rtp->getSeq();
    // _timestap = rtp->getStamp();
}
