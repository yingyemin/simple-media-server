#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "JT1078DecodeTrack.h"
#include "Logger.h"
#include "Util/String.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"
#include "Codec/AacTrack.h"
#include "Codec/G711Track.h"
#include "Codec/H264Frame.h"
#include "Codec/H265Frame.h"

using namespace std;

JT1078DecodeTrack::JT1078DecodeTrack(int trackIndex)
{
    _index = trackIndex;
	_state.index =0;
	_state.valprev =0;
    // _frame = make_shared<FrameBuffer>();
}

void JT1078DecodeTrack::onRtpPacket(const JT1078RtpPacket::Ptr& rtp)
{
    if (!_trackInfo && !_createInfoFailed) {
        if (rtp->getTrackType() == "video") {
            // if (rtp->getCodecType() == "h264") {
            //     auto trackInfo = H264Track::createTrack(_index, 96, 90000);
            //     _trackInfo = dynamic_pointer_cast<TrackInfo>(trackInfo);
            //     onTrackInfo(_trackInfo);
            //     _frame = make_shared<H264Frame>();
            //     _frame->_startSize = 4;
            // } else if (rtp->getCodecType() == "h265") {
            //     auto trackInfo = make_shared<H265Track>();
            //     trackInfo->codec_ = "h265";
            //     trackInfo->index_ = _index;
            //     trackInfo->trackType_ = "video";
            //     trackInfo->payloadType_ = 96;
            //     trackInfo->samplerate_ = 90000;
            //     _trackInfo = dynamic_pointer_cast<TrackInfo>(trackInfo);
            //     onTrackInfo(_trackInfo);
            //     _frame = make_shared<H265Frame>();
            //     _frame->_startSize = 4;
            // } else {
            //     // logInfo << "unsurpport video codec" << rtp->getCodecType();
            //     return ;
            // }
            _trackInfo = TrackInfo::createTrackInfo(rtp->getCodecType());
            if (_trackInfo) {
                onTrackInfo(_trackInfo);
            } else {
                logDebug << "创建TrackInfo失败";
                _createInfoFailed = true;
                return ;
            }

            int startSize = 0;
            if (rtp->getCodecType() == "h264" || rtp->getCodecType() == "h265" || rtp->getCodecType() == "h266") {
                startSize = 4;
            }

            _frame = FrameBuffer::createFrame(rtp->getCodecType(), startSize, VideoTrackType, 0);
        } else if (rtp->getTrackType() == "audio") {
            logDebug << "rtp->getCodecType() : " << rtp->getCodecType() << ", rtp->getCodecType() != adpcma = " << (rtp->getCodecType() != "adpcma");
            if (rtp->getCodecType() != "adpcma" && rtp->getCodecType() != "g726") {
                _trackInfo = TrackInfo::createTrackInfo(rtp->getCodecType());
            } else {
                _trackInfo = TrackInfo::createTrackInfo("g711a");
                _originAudioCodec = rtp->getCodecType();
            }
            if (_trackInfo) {
                onTrackInfo(_trackInfo);
            } else {
                logDebug << "创建TrackInfo失败";
                _createInfoFailed = true;
                return ;
            }

            int startSize = 0;
            if (rtp->getCodecType() == "aac") {
                startSize = 7;
            }

            _frame = FrameBuffer::createFrame(rtp->getCodecType(), startSize, AudioTrackType, 0);
            // if (rtp->getCodecType() == "g711a") {
            //     auto trackInfo = make_shared<G711aTrack>();
            //     trackInfo->codec_ = "g711a";
            //     trackInfo->index_ = _index;
            //     trackInfo->trackType_ = "audio";
            //     trackInfo->samplerate_ = 8000;
            //     _trackInfo = dynamic_pointer_cast<TrackInfo>(trackInfo);
            //     onTrackInfo(_trackInfo);
            // } else if (rtp->getCodecType() == "g711u") {
            //     auto trackInfo = make_shared<G711uTrack>();
            //     trackInfo->codec_ = "g711u";
            //     trackInfo->index_ = _index;
            //     trackInfo->trackType_ = "audio";
            //     trackInfo->samplerate_ = 8000;
            //     _trackInfo = dynamic_pointer_cast<TrackInfo>(trackInfo);
            //     onTrackInfo(_trackInfo);
            // } else if (rtp->getCodecType() == "aac") {
            //     auto trackInfo = make_shared<AacTrack>();
            //     trackInfo->codec_ = "aac";
            //     trackInfo->index_ = _index;
            //     trackInfo->trackType_ = "audio";
            //     trackInfo->payloadType_ = 97;
            //     trackInfo->samplerate_ = 44100;
            //     _trackInfo = dynamic_pointer_cast<TrackInfo>(trackInfo);
            //     onTrackInfo(_trackInfo);
            // } else {
            //     // logInfo << "unsurpport audio codec" << rtp->getCodecType();
            //     return ;
            // }
            // _frame = make_shared<FrameBuffer>();
        }
    }

    if (_onRtpPacket && _trackInfo) {
        _onRtpPacket(rtp, rtp->isStartGop());
    }
}

void JT1078DecodeTrack::startDecode()
{
    _startDecode = true;
}

void JT1078DecodeTrack::stopDecode()
{
    _startDecode = false;
}

void JT1078DecodeTrack::onFrame(const FrameBuffer::Ptr& frame)
{
    // static int ii = 0;
    // string name = "testjt1078split" + to_string(ii) + ".h264";
    // FILE* fp = fopen(name.data(), "ab+");
    // fwrite(frame->data(), 1, frame->size(), fp);
    // fclose(fp);

    logTrace << "stream path: " << _trackInfo->_parser.path_ << ", pts: " << frame->pts() << " size: " << frame->size();
    // if (!_ready && _trackInfo->trackType_ == "video") {
    //     if (_trackInfo->codec_ == "h264") {
    //         auto h264Frame = dynamic_pointer_cast<H264Frame>(frame);
    //         auto h264Track = dynamic_pointer_cast<H264Track>(_trackInfo);
    //         // logInfo << "h264 frame type: " << (int)h264Frame->getNalType();
    //         if (h264Frame->getNalType() == H264_SPS) {
    //             h264Track->setSps(h264Frame);
    //         } else if (h264Frame->getNalType() == H264_PPS) {
    //             h264Track->setPps(h264Frame);
    //         }

    //         if (h264Track->_sps && h264Track->_pps) {
    //             logInfo << "video ready ======================";
    //             _ready = true;
    //             if (_onReady) {
    //                 _onReady();
    //             }
    //         }
    //     } else if (_trackInfo->codec_ == "h265") {
    //         auto h265Frame = dynamic_pointer_cast<H265Frame>(frame);
    //         auto h265Track = dynamic_pointer_cast<H265Track>(_trackInfo);
    //         if (h265Frame->getNalType() == H265_SPS) {
    //             h265Track->setSps(h265Frame);
    //         } else if (h265Frame->getNalType() == H265_PPS) {
    //             h265Track->setPps(h265Frame);
    //         } else if (h265Frame->getNalType() == H265_VPS) {
    //             h265Track->setVps(h265Frame);
    //         }

    //         if (h265Track->_sps && h265Track->_pps && h265Track->_vps) {
    //             _ready = true;
    //             if (_onReady) {
    //                 _onReady();
    //             }
    //         }
    //     }
    // } else if (!_setAacCfg && _trackInfo->trackType_ == "audio") {
    //     if (_trackInfo->codec_ == "aac") {
    //         auto aacTrack = dynamic_pointer_cast<AacTrack>(_trackInfo);
    //         aacTrack->setAacInfo(string(frame->data(), 7));
    //         _setAacCfg = true;
    //         _ready = true;
    //         if (_onReady) {
    //             _onReady();
    //         }
    //     } else {
    //         _setAacCfg = true;
    //         _ready = true;
    //         logInfo << "audio ready ======================";
    //         if (_onReady) {
    //             _onReady();
    //         }
    //     }
    // }

    if (_originAudioCodec == "adpcma") {
        uint8_t* payload = (uint8_t*)frame->data();
        uint8_t* start = payload;
        uint64_t size = frame->size();
        if (payload[0] == 0x00 && payload[1] == 0x01 && 
                ((payload[2] & 0xff) == (size - 4) / 2) && payload[3] == 0x00)
        {
            payload += 8;
            size -= 8;

            _state.valprev = (uint16_t)(((start[5] << 8) & 0xff00) | (start[4] & 0xff));
            _state.index = start[6];
        } else {
            payload += 4;
            size -= 4;

            _state.valprev = (uint16_t)(((start[1] << 8) & 0xff00) | (start[0] & 0xff));
            _state.index = start[2];
        }

        uint16_t* pcm = new uint16_t[size*2];
        adpcm_decoder((char*)payload, (short*)pcm, size * 2, &_state);
        uint8_t* pcmdata = new uint8_t[size*4];
        for (int i = 0, k = 0; i < size*2; i++)
        {
            short s = pcm[i];
            pcmdata[k++] = (uint8_t)(s & 0xff);
            pcmdata[k++] = (uint8_t)((s >> 8) & 0xff);
        }

        auto newFrame = FrameBuffer::createFrame("g711a", 0, frame->_index, 0);
        // auto pcmFrame = make_shared<FrameBuffer>();
        // pcmFrame->_buffer.assign((char*)pcmdata, size * 4);
        
        newFrame->_buffer.resize(size * 2);
        _g711aEncode.pcm_2_alaw((const int16_t *)pcmdata, (uint8_t *)newFrame->data(), size * 2);

        newFrame->_dts = frame->_pts;
        newFrame->_index = _trackInfo->index_;
        newFrame->_codec = _trackInfo->codec_;
        newFrame->_trackType = _trackInfo->trackType_ == "video" ? VideoTrackType : AudioTrackType;
        if (_onFrame) {
            _onFrame(newFrame);
        }

        delete[] pcm;
        delete[] pcmdata;

        return ;
    } else if (_originAudioCodec == "g726") {
        int rateBits = 16;
        int bitrate = 0;
        uint64_t size = frame->size();
        uint64_t pcmSize;
        switch (rateBits) {
        case 16:
            bitrate = 8000 * 2;
            pcmSize = (2 * size * 120 + 30) / 30;
            break;
        case 24:
            bitrate = 8000 * 3;
            pcmSize = (2 * size * 80 + 30) / 30;
            break;
        case 32:
            bitrate = 8000 * 4;
            pcmSize = (2 * size * 60 + 30) / 30;
            break;
        case 40:
            bitrate = 8000 * 5;
            pcmSize = (2 * size * 40 + 30) / 30;
            break;
        default:
            break;
        }
        g726_init(&_state726, bitrate);
        uint8_t* pcmdata = new uint8_t[pcmSize];

        int result = g726_decode(&_state726, (short*)pcmdata, (const unsigned char*)frame->data(), size);
        auto newFrame = FrameBuffer::createFrame("g711a", 0, frame->_index, 0);
        // auto pcmFrame = make_shared<FrameBuffer>();
        // pcmFrame->_buffer.assign((char*)pcmdata, size * 4);
        
        newFrame->_buffer.resize(result);
        _g711aEncode.pcm_2_alaw((const int16_t *)pcmdata, (uint8_t *)newFrame->data(), result);

        newFrame->_dts = frame->_pts;
        newFrame->_index = _trackInfo->index_;
        newFrame->_codec = _trackInfo->codec_;
        newFrame->_trackType = AudioTrackType;
        if (_onFrame) {
            _onFrame(newFrame);
        }

        delete[] pcmdata;
        return ;
    }

    frame->_dts = frame->_pts;
    frame->_index = _trackInfo->index_;
    frame->_codec = _trackInfo->codec_;
    frame->_trackType = _trackInfo->trackType_ == "video" ? VideoTrackType : AudioTrackType;
    if (_onFrame) {
        _onFrame(frame);
    }
}

void JT1078DecodeTrack::createFrame()
{
    if (_trackInfo->codec_ == "h264") {
        _frame = make_shared<H264Frame>();
        _frame->_startSize = 4;
    } else if (_trackInfo->codec_ == "h265") {
        _frame = make_shared<H265Frame>();
        _frame->_startSize = 4;
    } else if (_trackInfo->codec_ == "aac" || _trackInfo->codec_ == "g711a" || _trackInfo->codec_ == "g711u") {
        _frame = make_shared<FrameBuffer>();
    }
}

void JT1078DecodeTrack::decodeRtp(const JT1078RtpPacket::Ptr& rtp)
{
    if (!rtp) {
        logDebug << "rtp is empty";
        return ;
    }

    if (!_trackInfo) {
        logDebug << "_trackInfo is empty, codec: " << rtp->getCodecType();
        return ;
    }

    if (rtp->getTrackType() != "audio" && rtp->getTrackType() != "video") {
        logInfo << " get a JT1078_Passthrough packet";
        return ;
    }

    auto subMark = rtp->getSubMark();
    // static int ii = 0;
    // string name = "testjt1078" + to_string(ii) + ".h264";
    // FILE* fp = fopen(name.c_str(), "ab+");

    logTrace << "stream path: " << _trackInfo->_parser.path_ << ", rtp->getSeq(): " << rtp->getSeq() << ", rtp->getTimestamp(): " << rtp->getTimestamp() 
            << ", codec : " << _trackInfo->codec_ << ", rtp subMark: " << (int)subMark;
    
    switch (subMark) {
    case JT1078_Atomic:
        _frame->_buffer.assign(rtp->getPayload(), rtp->getPayloadSize());
        _frame->_pts = rtp->getTimestamp();
        // _frame->_startSize = 4;
        // fwrite(_frame->data(), 1, _frame->size(), fp);
        // fclose(fp);
        _frame->split([this](const FrameBuffer::Ptr &subFrame){
            onFrame(subFrame);
        });
        createFrame();
        // ++ii;
        break;
    case JT1078_First:
        _lossRtp = false;
        _frame->_buffer.assign(rtp->getPayload(), rtp->getPayloadSize());
        break;
    case JT1078_Intermediate:
        // if (_lossRtp || rtp->getSeq() != (uint16_t)(_lastSeq + 1)) {
        //     _lossRtp = true;
        //     _frame->_buffer.clear();
        //     break;
        // }
        _frame->_buffer.append(rtp->getPayload(), rtp->getPayloadSize());
        break;
    case JT1078_Last:
        // if (_lossRtp || rtp->getSeq() != (uint16_t)(_lastSeq + 1)) {
        //     _lossRtp = true;
        //     _frame->_buffer.clear();
        //     break;
        // }
        _frame->_pts = rtp->getTimestamp();
        _frame->_buffer.append(rtp->getPayload(), rtp->getPayloadSize());
        // _frame->_startSize = 4;
        // fwrite(_frame->data(), 1, _frame->size(), fp);
        // fclose(fp);
        _frame->split([this](const FrameBuffer::Ptr &subFrame){
            onFrame(subFrame);
        });
        createFrame();
        // ++ii;
        break;
    default:
        logInfo << "invalid submark";
    }
        
    _lastSeq = rtp->getSeq();
}

void JT1078DecodeTrack::setOnTrackInfo(const function<void(const shared_ptr<TrackInfo>& trackInfo)>& cb)
{
    _onTrackInfo = cb;
}

void JT1078DecodeTrack::onTrackInfo(const shared_ptr<TrackInfo>& trackInfo)
{
    if (_onTrackInfo) {
        _onTrackInfo(trackInfo);
    }
}