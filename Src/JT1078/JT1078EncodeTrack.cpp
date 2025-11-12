#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#ifndef _WIN32
#include <arpa/inet.h>
#endif

#include "JT1078EncodeTrack.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "Util/Base64.h"
// #include "Codec/AacTrack.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"

using namespace std;

int getPayloadType(const string& codec) {
    if (codec == "aac") {
        return 19;
    } else if (codec == "g711a") {
        return 6;
    } else if (codec == "g711u") {
        return 7;
    } else if (codec == "adpcma") {
        return 26;
    } else if (codec == "h264") {
        return 98;
    } else if (codec == "h265") {
        return 99;
    } else {
        return 0;
    }
}

static void getSimNo(uint8_t* simNo, const string& simCode)
{
    int pos = 0;
    if (simCode.size() % 2 == 1) {
        auto num = simCode[pos] - '0';
        simNo[pos] = (num / 10) << 4 | (num % 10);
        ++pos;
    }

    for (int i = pos; i < simCode.size(); i += 2) {
        auto num = (simCode[i] - '0') * 10 + (simCode[i + 1] - '0');
        simNo[pos] = (num / 10) << 4 | (num % 10);
        ++pos;
    }
}

JT1078EncodeTrack::JT1078EncodeTrack(const shared_ptr<TrackInfo>& trackInfo, const string& simCode, int channel)
    :_trackInfo(trackInfo)
    ,_channel(channel)
    ,_simCode(simCode)
{
    logInfo << "JT1078EncodeTrack::JT1078EncodeTrack";
}

void JT1078EncodeTrack::startEncode()
{
    _startEncode = true;
}

void JT1078EncodeTrack::stopEncode()
{
    _startEncode = false;
}

void JT1078EncodeTrack::onFrame(const FrameBuffer::Ptr& frame)
{
    if (!_startEncode) {
        return ;
    }

    if (_firstPkt) {
        _firstPkt = false;
        _lastFrameStamp = frame->pts();
        _lastKeyframeStamp = frame->pts();
    }

    if (frame->size() <= 950) {
        createSingleRtp(frame);
    } else {
        createMultiRtp(frame);
    }

    _lastFrameStamp = frame->pts();
    if (frame->keyFrame()) {
        _lastKeyframeStamp = frame->pts();
    }
}

void JT1078EncodeTrack::createSingleRtp(const FrameBuffer::Ptr& frame)
{
    auto trackType = frame->getTrackType();
    int len;
    uint8_t dataType;
    if (trackType == VideoTrackType) {
        len = frame->size() + 30;
        if (frame->keyFrame()) {
            dataType = JT1078_VideoI;
        } else {
            dataType = JT1078_VideoP;
        }
    } else {
        len = frame->size() + 26;
        dataType = JT1078_Audio;
    }
    StreamBuffer::Ptr buffer = make_shared<StreamBuffer>(len + 1);
    auto rtp = make_shared<JT1078RtpPacket>(buffer, JT1078_2016);
    auto header = rtp->getHeader();
    rtp->type_ = _trackInfo->trackType_;
    rtp->trackIndex_ = _trackInfo->index_;

    buffer->data()[0] = 0x30;
    buffer->data()[1] = 0x31;
    buffer->data()[2] = 0x63;
    buffer->data()[3] = 0x64;
    header->version = 2;
    header->p = 0;
    header->extend = 0;
    header->cc = 1;
    header->mark = 1;
    header->pt = getPayloadType(_trackInfo->codec_);
    header->seq = htons(_seq++);
    getSimNo(header->simNo, _simCode);
    header->logicChannelNumber = _channel;
    header->dataType = dataType;
    header->subPackageHandleMark = JT1078_Atomic;

    int index = 16;
    writeUint32BE(buffer->data() + index, frame->pts() >> 32);
    index += 4;
    writeUint32BE(buffer->data() + index, frame->pts());
    index += 4;

    if (_trackInfo->trackType_ == "video") {
        writeUint16BE(buffer->data() + index, frame->pts() - _lastKeyframeStamp);
        writeUint16BE(buffer->data() + index + 2, frame->pts() - _lastFrameStamp);
        index += 4;
    }
    
    writeUint16BE(buffer->data() + index, frame->size());
    index += 2;

    memcpy(buffer->data() + index, frame->data(), frame->size());

    onRtpPacket(rtp);
}

void JT1078EncodeTrack::createMultiRtp(const FrameBuffer::Ptr& frame)
{
    auto trackType = frame->getTrackType();
    int headerLen;
    uint8_t dataType;
    if (trackType == VideoTrackType) {
        headerLen = 30;
        if (frame->keyFrame()) {
            dataType = JT1078_VideoI;
        } else {
            dataType = JT1078_VideoP;
        }
    } else {
        headerLen = 26;
        dataType = JT1078_Audio;
    }

    size_t length = frame->size();
    int frameIndex = 0;
    bool first = true;
    while (length > 0) {
        int rtpLen = length > 950 ? 950 : length;

        StreamBuffer::Ptr buffer = make_shared<StreamBuffer>(rtpLen + headerLen + 1);
        auto rtp = make_shared<JT1078RtpPacket>(buffer, JT1078_2016);
        auto header = rtp->getHeader();
        rtp->type_ = _trackInfo->trackType_;
        rtp->trackIndex_ = _trackInfo->index_;

        uint8_t subPackageHandleMark;
        if (first) {
            subPackageHandleMark = JT1078_First;
            first = false;
        } else if (length > 950) {
            subPackageHandleMark = JT1078_Intermediate;
        } else {
            subPackageHandleMark = JT1078_Last;
        }

        buffer->data()[0] = 0x30;
        buffer->data()[1] = 0x31;
        buffer->data()[2] = 0x63;
        buffer->data()[3] = 0x64;
        header->version = 2;
        header->p = 0;
        header->extend = 0;
        header->cc = 1;
        header->mark = length > 950 ? 0 : 1;
        header->pt = getPayloadType(_trackInfo->codec_);
        header->seq = htons(_seq++);
        getSimNo(header->simNo, _simCode);
        header->logicChannelNumber = _channel;
        header->dataType = dataType;
        header->subPackageHandleMark = subPackageHandleMark;

        int index = 16;
        writeUint32BE(buffer->data() + index, frame->pts() >> 32);
        index += 4;
        writeUint32BE(buffer->data() + index, frame->pts());
        index += 4;

        if (_trackInfo->trackType_ == "video") {
            writeUint16BE(buffer->data() + index, frame->pts() - _lastKeyframeStamp);
            writeUint16BE(buffer->data() + index + 2, frame->pts() - _lastFrameStamp);
            index += 4;
        }
        
        writeUint16BE(buffer->data() + index, rtpLen);
        index += 2;
        memcpy(buffer->data() + index, frame->data() + frameIndex, rtpLen);
        frameIndex += rtpLen;

        onRtpPacket(rtp);

        if (length <= 950) {
            break;
        } else {
            length -= 950;
        }
    }
}

void JT1078EncodeTrack::onRtpPacket(const JT1078RtpPacket::Ptr& rtp)
{
    // auto data = rtp->data();
    // data[0] = (rtp->size() - 2) >> 8;
    // data[1] = (rtp->size() - 2) & 0x00FF;

    if (_onRtpPacket) {
        _onRtpPacket(rtp);
    }
    // logInfo << "encode a rtp packet: " << rtp->size();
    // _ssrc = rtp->getSSRC();
    // _seq = rtp->getSeq();
    // _timestap = rtp->getStamp();
}
