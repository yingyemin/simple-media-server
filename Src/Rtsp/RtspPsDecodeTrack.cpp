#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtspPsDecodeTrack.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/Base64.h"
#include "Codec/AacTrack.h"
#include "Codec/G711Track.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"
#include "Codec/H265Frame.h"
#include "Codec/H264Frame.h"
#include "Rtp/Decoder/RtpDecodeH264.h"
#include "Rtp/Decoder/RtpDecodeH265.h"

using namespace std;

static shared_ptr<TrackInfo> createTrackBySdp(const shared_ptr<SdpMedia>& media)
{
    logInfo << "createTrackBySdp, codec: " << media->codec_;
    shared_ptr<TrackInfo> trackInfo;
    if (strcasecmp(media->codec_.data(), "mpeg4-generic") == 0) {
        auto aacTrackInfo = make_shared<AacTrack>();
        string aac_cfg_str = findSubStr(media->fmtp_, "config=", "");
        if (aac_cfg_str.empty()) {
            aac_cfg_str = findSubStr(media->fmtp_, "config=", ";");
        }
        if (aac_cfg_str.empty()) {
            //如果sdp中获取不到aac config信息，那么在rtp也无法获取，那么忽略该Track
            return nullptr;
        }
        string aac_cfg;
        for(int i = 0 ; i < aac_cfg_str.size() / 2 ; ++i ){
            unsigned int cfg;
            sscanf(aac_cfg_str.substr(i * 2, 2).data(), "%02X", &cfg);
            cfg &= 0x00FF;
            aac_cfg.push_back((char)cfg);
        }
        aacTrackInfo->setAacInfo(aac_cfg);
        trackInfo = aacTrackInfo;
        trackInfo->codec_ = "aac";
    } else if (strcasecmp(media->codec_.data(), "pcma") == 0) {
        auto g711aTrackInfo = make_shared<G711aTrack>();
        trackInfo = g711aTrackInfo;
        trackInfo->codec_ = "g711a";
    } else if (strcasecmp(media->codec_.data(), "pcmu") == 0) {
        auto g711uTrackInfo = make_shared<G711uTrack>();
        trackInfo = g711uTrackInfo;
        trackInfo->codec_ = "g711u";
    } else if (strcasecmp(media->codec_.data(), "h264") == 0) {
        logInfo << "createTrackBySdp h264";
        auto h264TrackInfo = make_shared<H264Track>();
        //a=fmtp:96 packetization-mode=1;profile-level-id=42C01F;sprop-parameter-sets=Z0LAH9oBQBboQAAAAwBAAAAPI8YMqA==,aM48gA==
        auto mapFmtp = split(findSubStr(media->fmtp_," ", ""),";","=");
        auto sps_pps = mapFmtp["sprop-parameter-sets"];
        string base64_SPS = findSubStr(sps_pps.data(), "", ",");
        string base64_PPS = findSubStr(sps_pps.data(), ",", "");

        logInfo << "createTrackBySdp start decode base64, sps: " << base64_SPS << ", pps: " << base64_PPS;

        auto spsFrame = make_shared<H264Frame>();
        spsFrame->_startSize = 4;
        spsFrame->_buffer.assign("\x00\x00\x00\x01", 4);
        spsFrame->_buffer.append(Base64::decode(base64_SPS));
        // FILE* fp = fopen("sps.h264", "wb");
        // fwrite(spsFrame->_buffer.data(), 1, spsFrame->_buffer.size(), fp);
        // fclose(fp);
        
        auto ppsFrame = make_shared<FrameBuffer>();
        ppsFrame->_startSize = 4;
        ppsFrame->_buffer.assign("\x00\x00\x00\x01", 4);
        ppsFrame->_buffer.append(Base64::decode(base64_PPS));
        // FILE* pfp = fopen("pps.h264", "wb");
        // fwrite(ppsFrame->_buffer.data(), 1, ppsFrame->_buffer.size(), pfp);
        // fclose(pfp);
        
        h264TrackInfo->setSps(spsFrame);
        h264TrackInfo->setPps(ppsFrame);
        logInfo << "createTrackBySdp start trackInfo";
        trackInfo = h264TrackInfo;
        trackInfo->codec_ = "h264";
        logInfo << "createTrackBySdp trackInfo";
    } else if (strcasecmp(media->codec_.data(), "h265") == 0) {
        auto h265TrackInfo = make_shared<H265Track>();
        //a=fmtp:96 sprop-sps=QgEBAWAAAAMAsAAAAwAAAwBdoAKAgC0WNrkky/AIAAADAAgAAAMBlQg=; sprop-pps=RAHA8vA8kAA=
        auto mapFmtp = split(findSubStr(media->fmtp_," ", ""),";","=");
        auto vpsFrame = make_shared<FrameBuffer>();
        vpsFrame->_startSize = 4;
        vpsFrame->_buffer.assign("\x00\x00\x00\x01", 4);
        vpsFrame->_buffer.append(Base64::decode(mapFmtp["sprop-vps"]));
        
        auto spsFrame = make_shared<FrameBuffer>();
        spsFrame->_startSize = 4;
        spsFrame->_buffer.assign("\x00\x00\x00\x01", 4);
        spsFrame->_buffer.append(Base64::decode(mapFmtp["sprop-sps"]));

        auto ppsFrame = make_shared<FrameBuffer>();
        ppsFrame->_startSize = 4;
        ppsFrame->_buffer.assign("\x00\x00\x00\x01", 4);
        ppsFrame->_buffer.append(Base64::decode(mapFmtp["sprop-pps"]));

        h265TrackInfo->setVps(vpsFrame);
        h265TrackInfo->setSps(spsFrame);
        h265TrackInfo->setPps(ppsFrame);
        trackInfo = h265TrackInfo;
        trackInfo->codec_ = "h265";
    }

    if (trackInfo) {
        trackInfo->payloadType_ = media->payloadType_;
        trackInfo->samplerate_ = media->samplerate_;
        trackInfo->index_ = media->index_;
        trackInfo->channel_ = media->channel_;
        trackInfo->trackType_ = media->trackType_;
        // trackInfo->codec_ = media->codec_;
    }

    return trackInfo;
}

RtspPsDecodeTrack::RtspPsDecodeTrack(int trackIndex, const shared_ptr<SdpMedia>& media)
    :_index(trackIndex)
    ,_type(media->trackType_ == "video" ? VideoTrackType : AudioTrackType)
    ,_media(media)
{
    _trackInfo = make_shared<TrackInfo>();
    _trackInfo->codec_ = "ps";
    _trackInfo->index_ = media->index_;
    _trackInfo->samplerate_ = media->samplerate_;

    // _trackInfo = createTrackBySdp(media);
    if (_type == VideoTrackType) {
        _stampAdjust = make_shared<VideoStampAdjust>(25);
    } else {
        _stampAdjust = make_shared<AudioStampAdjust>(0);
    }
}

void RtspPsDecodeTrack::onRtpPacket(const RtpPacket::Ptr& rtp, bool start)
{
    // if (_trackInfo->trackType_ == "video") {
    //     if (_trackInfo->codec_ == "h264") {
    //         start = RtpDecodeH264::isStartGop(rtp);
    //     } else if (_trackInfo->codec_ == "h265") {
    //         start = RtpDecodeH265::isStartGop(rtp);
    //     } else {
    //         start = true;
    //     }
    // } else {
    //     start = false;
    // }

    start = true;
    if (_onRtpPacket) {
        _onRtpPacket(rtp, start);
    }

    _ssrc = rtp->getSSRC();
    _seq = rtp->getSeq();
    _timestap = rtp->getStamp();
}

void RtspPsDecodeTrack::startDecode()
{
    if (!_trackInfo) {
        return ;
    }
    
    weak_ptr<RtspPsDecodeTrack> wSelf = shared_from_this();
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

void RtspPsDecodeTrack::stopDecode()
{
    _startdecode = false;
    _startDemuxer = false;
}

void RtspPsDecodeTrack::decodeRtp(const RtpPacket::Ptr& rtp)
{
    if (!_startdecode) {
        _decoder = nullptr;
        return;
    }

    if (_decoder) {
        _decoder->decode(rtp);
    }
}

void RtspPsDecodeTrack::onPsFrame(const FrameBuffer::Ptr frame)
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

void RtspPsDecodeTrack::onFrame(const FrameBuffer::Ptr& frame)
{
    int samples = 1;
    if (frame->getTrackType() == AudioTrackType) {
        if (frame->codec() == "aac") {
            samples = 1024;
        } else if (frame->codec() == "g711a" || frame->codec() == "g711u") {
            samples = frame->size() - frame->startSize();
        }
    }
    // _stampAdjust->inputStamp(frame->_pts, frame->_dts, samples);
    if (_onFrame) {
        _onFrame(frame);
    }
    // FILE* fp = fopen("test.h264", "ab+");
    // fwrite(frame->_buffer.data(), 1, frame->_buffer.size(), fp);
    // fclose(fp);
    // logInfo << "decode a frame: " << frame->size();
}

void RtspPsDecodeTrack::setOnTrackInfo(const function<void(const shared_ptr<TrackInfo>& trackInfo)>& cb)
{
    _onTrackInfo = cb;
}

void RtspPsDecodeTrack::setOnReady(const function<void()>& cb)
{
    _onReady = cb;
}