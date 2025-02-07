#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtspTrack.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/Base64.h"
#include "Codec/AacTrack.h"
#include "Codec/G711Track.h"
#include "Codec/Mp3Track.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"
#include "Codec/OpusTrack.h"
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
    } else if (strcasecmp(media->codec_.data(), "opus") == 0) {
        auto opusTrackInfo = make_shared<OpusTrack>();
        trackInfo = opusTrackInfo;
        trackInfo->codec_ = "g711u";
    } else if (strcasecmp(media->codec_.data(), "mp3") == 0) {
        auto mp3Track = make_shared<Mp3Track>();
        trackInfo = mp3Track;
        trackInfo->codec_ = "mp3";
    } else if (strcasecmp(media->codec_.data(), "h264") == 0) {
        logInfo << "createTrackBySdp h264";
        auto h264TrackInfo = H264Track::createTrack(media->index_, media->payloadType_, media->samplerate_);
        //a=fmtp:96 packetization-mode=1;profile-level-id=42C01F;sprop-parameter-sets=Z0LAH9oBQBboQAAAAwBAAAAPI8YMqA==,aM48gA==
        // auto mapFmtp = split(findSubStr(media->fmtp_," ", ""),";","=");
        auto mapFmtp = split(media->fmtp_,";","=");
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
        
        auto ppsFrame = make_shared<H264Frame>();
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
        // trackInfo->codec_ = "h264";
        logInfo << "createTrackBySdp trackInfo";
    } else if (strcasecmp(media->codec_.data(), "h265") == 0) {
        auto h265TrackInfo = make_shared<H265Track>();
        //a=fmtp:96 sprop-sps=QgEBAWAAAAMAsAAAAwAAAwBdoAKAgC0WNrkky/AIAAADAAgAAAMBlQg=; sprop-pps=RAHA8vA8kAA=
        // auto mapFmtp = split(findSubStr(media->fmtp_," ", ""),";","=");
        auto mapFmtp = split(media->fmtp_,";","=");
        auto vpsFrame = make_shared<H265Frame>();
        vpsFrame->_startSize = 4;
        vpsFrame->_buffer.assign("\x00\x00\x00\x01", 4);
        vpsFrame->_buffer.append(Base64::decode(mapFmtp["sprop-vps"]));
        
        auto spsFrame = make_shared<H265Frame>();
        spsFrame->_startSize = 4;
        spsFrame->_buffer.assign("\x00\x00\x00\x01", 4);
        spsFrame->_buffer.append(Base64::decode(mapFmtp["sprop-sps"]));

        auto ppsFrame = make_shared<H265Frame>();
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
        if (media->samplerate_ > 0) {
            trackInfo->samplerate_ = media->samplerate_;
        }
        trackInfo->index_ = media->index_;
        if (media->channel_ > 0) {
            trackInfo->channel_ = media->channel_;
        }
        trackInfo->trackType_ = media->trackType_;
        // trackInfo->codec_ = media->codec_;
    }

    return trackInfo;
}

RtspDecodeTrack::RtspDecodeTrack(int trackIndex, const shared_ptr<SdpMedia>& media)
    :_index(trackIndex)
    ,_type(media->trackType_ == "video" ? VideoTrackType : AudioTrackType)
    ,_media(media)
{
    _trackInfo = createTrackBySdp(media);
    if (_type == VideoTrackType) {
        _stampAdjust = make_shared<VideoStampAdjust>(25);
    } else {
        _stampAdjust = make_shared<AudioStampAdjust>(0);
    }
}

void RtspDecodeTrack::onRtpPacket(const RtpPacket::Ptr& rtp, bool start)
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

    if (_trackInfo->trackType_ == "video") {
        if (_trackInfo->codec_ == "h264") {
            start = RtpDecodeH264::isStartGop(rtp);
        } else if (_trackInfo->codec_ == "h265") {
            start = RtpDecodeH265::isStartGop(rtp);
        } else {
            start = true;
        }
    } else {
        start = false;
    }

    if (_onRtpPacket) {
        _onRtpPacket(rtp, start);
    }

    _ssrc = rtp->getSSRC();
    _seq = rtp->getSeq();
    _timestap = rtp->getStamp();
}

void RtspDecodeTrack::startDecode()
{
    if (!_trackInfo) {
        return ;
    }
    
    if (!_decoder) {
        weak_ptr<RtspTrack> wSelf = shared_from_this();
        _decoder = RtpDecoder::creatDecoder(_trackInfo);
        if (!_decoder) {
            logInfo << "不支持的编码：" << _trackInfo->codec_;
            throw "不支持的编码";
        }
        _decoder->setOnDecode([wSelf](const FrameBuffer::Ptr& frame){
            auto self = wSelf.lock();
            self->onFrame(frame);
        });
    }
}

void RtspDecodeTrack::stopDecode()
{
    _decoder = nullptr;
}

void RtspDecodeTrack::decodeRtp(const RtpPacket::Ptr& rtp)
{
    if (_decoder) {
        _decoder->decode(rtp);
    }
}

void RtspDecodeTrack::onFrame(const FrameBuffer::Ptr& frame)
{
    int samples = 1;
    if (_type == AudioTrackType) {
        if (_trackInfo->codec_ == "aac") {
            samples = 1024;
        } else if (_trackInfo->codec_ == "g711a" || _trackInfo->codec_ == "g711u") {
            samples = frame->size() - frame->startSize();
        }
    }
    // logInfo << "before adjust frame pts: " << frame->_pts << ", frame dts: " << frame->_dts;
    // _stampAdjust->inputStamp(frame->_pts, frame->_dts, samples);
    // logInfo << "frame pts: " << frame->_pts << ", frame dts: " << frame->_dts;
    if (_onFrame) {
        _onFrame(frame);
    }
    // logInfo << "decode a frame";
}


//////////////////////RtspEncodeTrack///////////////////


RtspEncodeTrack::RtspEncodeTrack(int trackIndex, const shared_ptr<TrackInfo>& trackInfo)
    :_index(trackIndex)
    ,_type(trackInfo->trackType_ == "video" ? VideoTrackType : AudioTrackType)
    ,_trackInfo(trackInfo)
{
    _trackInfo->ssrc_ = _trackInfo->index_;
    _media = make_shared<SdpMedia>();
    _media->payloadType_ = trackInfo->payloadType_;
    _media->samplerate_ = trackInfo->samplerate_;
    _media->index_ = trackInfo->index_;
    _media->channel_ = trackInfo->channel_;
    _media->trackType_ = trackInfo->trackType_;
    _media->codec_ = trackInfo->codec_;
}

void RtspEncodeTrack::startEncode()
{
    logInfo << "RtspEncodeTrack::startEncode";
    weak_ptr<RtspEncodeTrack> wSelf = dynamic_pointer_cast<RtspEncodeTrack>(shared_from_this());
    if (!_encoder) {
        _encoder = RtpEncoder::create(_trackInfo);
        _encoder->setSsrc(_ssrc);
        _encoder->setOnRtpPacket([wSelf](const RtpPacket::Ptr& rtp, bool start){
            auto self = wSelf.lock();
            self->onRtpPacket(rtp, start);
        });

        _encoder->setEnableHuge(_enableHuge);
        _encoder->setFastPts(_enableFastPts);
    }
}

void RtspEncodeTrack::setEnableHuge(bool enabled)
{
    RtspTrack::setEnableHuge(enabled);

    if (_encoder) {
        _encoder->setEnableHuge(enabled);
    }
}

void RtspEncodeTrack::setFastPts(bool enabled)
{
    RtspTrack::setFastPts(enabled);

    if (_encoder) {
        _encoder->setFastPts(enabled);
    }
}

void RtspEncodeTrack::onFrame(const FrameBuffer::Ptr& frame)
{
    if (_encoder) {
        // logInfo << "encode a frame";
        // if (frame->getNalType() < 30)
        //     logInfo << "is h265 b frame: " << _trackInfo->isBFrame((unsigned char*)frame->data() + frame->startSize(), frame->size() - frame->startSize());
        _encoder->encode(frame);
    }
}

void RtspEncodeTrack::onRtpPacket(const RtpPacket::Ptr& rtp, bool start)
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
