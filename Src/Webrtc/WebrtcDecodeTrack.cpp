#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "WebrtcDecodeTrack.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/Base64.h"
#include "Codec/AacTrack.h"
#include "Codec/G711Track.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"
#include "Codec/H265Frame.h"
#include "Codec/H264Frame.h"

using namespace std;

static shared_ptr<TrackInfo> createTrackBySdp(int index, const shared_ptr<WebrtcPtInfo>& piInfo)
{
    logInfo << "createTrackBySdp, codec: " << piInfo->codec_;
    shared_ptr<TrackInfo> trackInfo;
    if (strcasecmp(piInfo->codec_.data(), "mpeg4-generic") == 0) {
        auto aacTrackInfo = make_shared<AacTrack>();
        // string aac_cfg_str = findSubStr(piInfo->fmtp_, "config=", "");
        // if (aac_cfg_str.empty()) {
        //     aac_cfg_str = findSubStr(piInfo->fmtp_, "config=", ";");
        // }
        // if (aac_cfg_str.empty()) {
        //     //如果sdp中获取不到aac config信息，那么在rtp也无法获取，那么忽略该Track
        //     return nullptr;
        // }
        // string aac_cfg;
        // for(int i = 0 ; i < aac_cfg_str.size() / 2 ; ++i ){
        //     unsigned int cfg;
        //     sscanf(aac_cfg_str.substr(i * 2, 2).data(), "%02X", &cfg);
        //     cfg &= 0x00FF;
        //     aac_cfg.push_back((char)cfg);
        // }
        // aacTrackInfo->setAacInfo(aac_cfg);
        trackInfo = aacTrackInfo;
        trackInfo->codec_ = "aac";
        trackInfo->trackType_ = "audio";
    } else if (strcasecmp(piInfo->codec_.data(), "pcma") == 0) {
        auto g711aTrackInfo = make_shared<G711aTrack>();
        trackInfo = g711aTrackInfo;
        trackInfo->codec_ = "g711a";
        trackInfo->trackType_ = "audio";
    } else if (strcasecmp(piInfo->codec_.data(), "pcmu") == 0) {
        auto g711uTrackInfo = make_shared<G711uTrack>();
        trackInfo = g711uTrackInfo;
        trackInfo->codec_ = "g711u";
        trackInfo->trackType_ = "audio";
    } else if (strcasecmp(piInfo->codec_.data(), "h264") == 0) {
        logInfo << "createTrackBySdp h264";
        auto h264TrackInfo = make_shared<H264Track>();
        //a=fmtp:96 packetization-mode=1;profile-level-id=42C01F;sprop-parameter-sets=Z0LAH9oBQBboQAAAAwBAAAAPI8YMqA==,aM48gA==
        // auto mapFmtp = split(findSubStr(piInfo->fmtp_," ", ""),";","=");
        // auto sps_pps = mapFmtp["sprop-parameter-sets"];
        // string base64_SPS = findSubStr(sps_pps.data(), "", ",");
        // string base64_PPS = findSubStr(sps_pps.data(), ",", "");

        // logInfo << "createTrackBySdp start decode base64, sps: " << base64_SPS << ", pps: " << base64_PPS;

        // auto spsFrame = make_shared<H264Frame>();
        // spsFrame->_startSize = 4;
        // spsFrame->_buffer.assign("\x00\x00\x00\x01", 4);
        // spsFrame->_buffer.append(Base64::decode(base64_SPS));
        // // FILE* fp = fopen("sps.h264", "wb");
        // // fwrite(spsFrame->_buffer.data(), 1, spsFrame->_buffer.size(), fp);
        // // fclose(fp);
        
        // auto ppsFrame = make_shared<FrameBuffer>();
        // ppsFrame->_startSize = 4;
        // ppsFrame->_buffer.assign("\x00\x00\x00\x01", 4);
        // ppsFrame->_buffer.append(Base64::decode(base64_PPS));
        // // FILE* pfp = fopen("pps.h264", "wb");
        // // fwrite(ppsFrame->_buffer.data(), 1, ppsFrame->_buffer.size(), pfp);
        // // fclose(pfp);
        
        // h264TrackInfo->setSps(spsFrame);
        // h264TrackInfo->setPps(ppsFrame);
        logInfo << "createTrackBySdp start trackInfo";
        trackInfo = h264TrackInfo;
        trackInfo->codec_ = "h264";
        trackInfo->trackType_ = "video";
        logInfo << "createTrackBySdp trackInfo";
    } else if (strcasecmp(piInfo->codec_.data(), "h265") == 0) {
        auto h265TrackInfo = make_shared<H265Track>();
        //a=fmtp:96 sprop-sps=QgEBAWAAAAMAsAAAAwAAAwBdoAKAgC0WNrkky/AIAAADAAgAAAMBlQg=; sprop-pps=RAHA8vA8kAA=
        // auto mapFmtp = split(findSubStr(piInfo->fmtp_," ", ""),";","=");
        // auto vpsFrame = make_shared<FrameBuffer>();
        // vpsFrame->_startSize = 4;
        // vpsFrame->_buffer.assign("\x00\x00\x00\x01", 4);
        // vpsFrame->_buffer.append(Base64::decode(mapFmtp["sprop-vps"]));
        
        // auto spsFrame = make_shared<FrameBuffer>();
        // spsFrame->_startSize = 4;
        // spsFrame->_buffer.assign("\x00\x00\x00\x01", 4);
        // spsFrame->_buffer.append(Base64::decode(mapFmtp["sprop-sps"]));

        // auto ppsFrame = make_shared<FrameBuffer>();
        // ppsFrame->_startSize = 4;
        // ppsFrame->_buffer.assign("\x00\x00\x00\x01", 4);
        // ppsFrame->_buffer.append(Base64::decode(mapFmtp["sprop-pps"]));

        // h265TrackInfo->setVps(vpsFrame);
        // h265TrackInfo->setSps(spsFrame);
        // h265TrackInfo->setPps(ppsFrame);
        trackInfo = h265TrackInfo;
        trackInfo->codec_ = "h265";
        trackInfo->trackType_ = "video";
    }

    if (trackInfo) {
        trackInfo->payloadType_ = piInfo->payloadType_;
        trackInfo->samplerate_ = piInfo->samplerate_;
        trackInfo->index_ = index;
        // trackInfo->channel_ = piInfo->channel_;
        // trackInfo->trackType_ = piInfo->trackType_;
        // trackInfo->codec_ = media->codec_;
    }

    return trackInfo;
}

WebrtcDecodeTrack::WebrtcDecodeTrack(int trackIndex, int trackType, const shared_ptr<WebrtcPtInfo>& ptInfo)
    :_index(trackIndex)
    ,_type(trackType)
    ,_ptInfo(ptInfo)
{
    _trackInfo = createTrackBySdp(trackIndex, ptInfo);
    if (_type == VideoTrackType) {
        _stampAdjust = make_shared<VideoStampAdjust>(25);
    } else {
        _stampAdjust = make_shared<AudioStampAdjust>(0);
    }
}

void WebrtcDecodeTrack::onRtpPacket(const RtpPacket::Ptr& rtp)
{
    // TODO 将demux和mux分成两个类

    // 解封装的时候才需要除一下
    // if (_media->samplerate_ > 0) {
    //     rtp->samplerate_ = _media->samplerate_;
    //     rtp->getHeader()->stamp /= rtp->samplerate_;
    // }
    // if (_decoder) {
    //     _decoder->decode(rtp);
    // }
    // _ring->write(rtp, true);

    if (_onRtpPacket) {
        _onRtpPacket(rtp);
    }
}

void WebrtcDecodeTrack::startDecode()
{
    logTrace << "WebrtcDecodeTrack::startDecode()";
    _enableDecode = true;
    if (!_decoder) {
        weak_ptr<WebrtcDecodeTrack> wSelf = shared_from_this();
        _decoder = RtpDecoder::creatDecoder(_trackInfo);
        if (!_decoder) {
            logInfo << "不支持的编码：" << _trackInfo->codec_;
            throw "不支持的编码";
        }
        _decoder->setOnDecode([wSelf](const FrameBuffer::Ptr& frame){
            auto self = wSelf.lock();
            if (self)
                self->onFrame(frame);
        });
    }
}

void WebrtcDecodeTrack::stopDecode()
{
    _enableDecode = false;
}

void WebrtcDecodeTrack::decodeRtp(const RtpPacket::Ptr& rtp)
{
    if (!_enableDecode) {
        _decoder = nullptr;
        return ;
    }
    if (_decoder) {
        _decoder->decode(rtp);
    }
}

void WebrtcDecodeTrack::onFrame(const FrameBuffer::Ptr& frame)
{
    int samples = 1;
    if (_type == AudioTrackType) {
        if (_trackInfo->codec_ == "aac") {
            samples = 1024;
        } else if (_trackInfo->codec_ == "g711a") {
            samples = frame->size() - frame->startSize();
        }
    } else {
        if (!_ready) {
            if (_trackInfo->codec_ == "h264") {
                auto h264Track = dynamic_pointer_cast<H264Track>(_trackInfo);
                auto h264Frame = dynamic_pointer_cast<H264Frame>(frame);
                if (!h264Track->_sps || !h264Track->_pps) {
                    // set sps pps
                    if (frame->getNalType() == H264_SPS) {
                        h264Track->setSps(frame);
                        if (h264Track->_pps) {
                            if (_onReady) {
                                _onReady();
                            }
                            _ready = true;
                        }
                    } else if (frame->getNalType() == H264_PPS) {
                        h264Track->setPps(frame);
                        if (h264Track->_sps) {
                            if (_onReady) {
                                _onReady();
                            }
                            _ready = true;
                        }
                    }
                }
            }
        }
    }
    // _stampAdjust->inputStamp(frame->_pts, frame->_dts, samples);
    if (_onFrame) {
        _onFrame(frame);
    }
    // logInfo << "decode a frame";
}