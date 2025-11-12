#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <cmath>

#include "H264Track.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "Util/Base64.h"
#include "H264Nal.h"
#include "H264Frame.h"

using namespace std;

H264Track::H264Track()
{

}

void H264Track::setPps(const FrameBuffer::Ptr& pps)
{
    // if (!pps) {
    //     return ;
    // }

    pps->split([this](const FrameBuffer::Ptr& subFrame) {
        if (subFrame->getNalType() == 8) {
            _pps = subFrame;
        }
    });

    // while (_pps->size() - _pps->startSize() < 5) {
    //     _pps->_buffer.append("\x00", 1);
    // }
}

string H264Track::getSdp()
{
    stringstream ss;
    ss << "m=video 0 RTP/AVP " << payloadType_ << "\r\n"
    // ss << "b=AS:" << bitrate << "\r\n";
       << "a=rtpmap:" << payloadType_ << " H264/" << samplerate_ << "\r\n"
       << "a=fmtp:" << payloadType_ << " packetization-mode=1";

    if (_sps && _pps && _sps->size() > _sps->startSize() && _pps->size() > _pps->startSize()) {
        char strTemp[100];
        uint32_t profile_level_id = 0;
        int length = _sps->size() - _sps->startSize();
        auto sps = string(_sps->data() + _sps->startSize(), _sps->size() - _sps->startSize());
        auto pps = string(_pps->data() + _pps->startSize(), _pps->size() - _pps->startSize());
        if (length >= 4) { // sanity check
            profile_level_id = (uint8_t(sps[1]) << 16) |
                                (uint8_t(sps[2]) << 8)  |
                                (uint8_t(sps[3])); // profile_idc|constraint_setN_flag|level_idc
        }
        memset(strTemp, 0, 100);
        sprintf(strTemp, "%06X", profile_level_id);
        ss << "; profile-level-id=" << strTemp;
        ss << "; sprop-parameter-sets=";
        ss << Base64::encode(sps) << ",";
        ss << Base64::encode(pps) << "\r\n";
    } else {
        ss << "\r\n";
    }
    ss << "a=control:trackID=" << index_ << "\r\n";

    return ss.str();
}

void H264Track::getWidthAndHeight(int& width, int& height, int& fps)
{
    if (_width && _height) {
        width = _width;
        height = _height;
        fps = fps_;

        return ;
    }
    if (!_sps || !_pps) {
        logWarn << "sps or pps is null";
        return ;
    }
    auto sps = _sps->data() + _sps->startSize();
    auto size = _sps->size() - _sps->startSize();
    if (size <= 0) {
        return ;
    }
    
    auto spsBuffer = new char[size];
    memcpy(spsBuffer, sps, size);
    h264_decode_sps((unsigned char*)spsBuffer, size, _width, _height, fps_);
    delete []spsBuffer;

    width = _width;
    height = _height;
    fps = fps_;
}

string H264Track::getConfig()
{
    if (!_sps || !_pps) {
        return "";
    }
    
    string config;

    auto spsSize = _sps->startSize();
    auto ppsSize = _pps->startSize();

    auto spsBuffer = _sps->_buffer;
    int spsLen = _sps->size() - spsSize;
    auto ppsBuffer = _pps->_buffer;
    int ppsLen = _pps->size() - ppsSize;

    if (spsLen <= 0 || ppsLen <= 0 || spsBuffer->size() < spsSize + 3) {
        return "";
    }

    config.resize(11 + spsLen + ppsLen);
    auto data = (char*)config.data();

    // *data++ = 0x17; //key frame, AVC
    // *data++ = 0x00; //avc sequence header
    // *data++ = 0x00; //composit time 
    // *data++ = 0x00; //composit time
    // *data++ = 0x00; //composit time
    *data++ = 0x01;   //configurationversion
    *data++ = spsBuffer->data()[spsSize + 1]; //avcprofileindication
    *data++ = spsBuffer->data()[spsSize + 2]; //profilecompatibilty
    *data++ = spsBuffer->data()[spsSize + 3]; //avclevelindication
    *data++ = 0xff;   //reserved + lengthsizeminusone
    *data++ = 0xe1;   //num of sps
    *data++ = (uint8_t)(spsLen >> 8); //sequence parameter set length high 8 bits
    *data++ = (uint8_t)(spsLen); //sequence parameter set  length low 8 bits
    // data length 13
    memcpy(data, spsBuffer->data() + spsSize, spsLen); //H264 sequence parameter set
    data += spsLen; // 13 + spsLen

    *data++ = 0x01; //num of pps
    *data++ = (uint8_t)(ppsLen >> 8); //picture parameter set length high 8 bits
    *data++ = (uint8_t)(ppsLen); //picture parameter set length low 8 bits
    // 16 + spsLen + ppsLen
    memcpy(data, ppsBuffer->data() + ppsSize, ppsLen); //H264 picture parameter set

    return config;
}

void H264Track::setConfig(const string& config)
{
    _avcc = config;
    if (config.size() < 8) {
        return ;
    }

    int spsNum = config[5] & 0x1f;
    int spsCount = 0;
    int index = 6;
    while (spsCount < spsNum) {
        // 这里应该判断一下，剩余长度够不够
        int spsLen =(config[index] & 0x000000FF) << 8 | (config[index+1] & 0x000000FF);
        index += 2;
        auto frame = make_shared<H264Frame>();
        frame->_startSize = 4;
        frame->_codec = "h264";
        frame->_index = index_;
        frame->_trackType = VideoTrackType;

        frame->_buffer->assign("\x00\x00\x00\x01", 4);
        frame->_buffer->append((char*)config.data() + index, spsLen);
        frame->_pts = frame->_dts = 0;
        setSps(frame);

        index += spsLen;
        ++spsCount;
    }

    // pps
    int ppsNum = config[index] & 0x1f;
    int ppsCount = 0;
    index += 1;
    while (ppsCount < ppsNum) {
        int ppsLen =(config[index] & 0x000000FF) << 8 | (config[index+1] & 0x000000FF);
        index += 2;
        auto frame = make_shared<H264Frame>();
        frame->_startSize = 4;
        frame->_codec = "h264";
        frame->_index = index_;
        frame->_trackType = VideoTrackType;

        frame->_buffer->assign("\x00\x00\x00\x01", 4);
        frame->_buffer->append((char*)config.data() + index, ppsLen);
        frame->_pts = frame->_dts = 0;

        setPps(frame);

        index += ppsLen;
        ++ppsCount;
    }
}

H264Track::Ptr H264Track::createTrack(int index, int payloadType, int samplerate)
{
    auto trackInfo = make_shared<H264Track>();
    trackInfo->index_ = index;
    trackInfo->codec_ = "h264";
    trackInfo->payloadType_ = payloadType;
    trackInfo->trackType_ = "video";
    trackInfo->samplerate_ = samplerate;

    return trackInfo;
}

void H264Track::registerTrackInfo()
{
	TrackInfo::registerTrackInfo("h264", [](int index, int payloadType, int samplerate){
		auto trackInfo = make_shared<H264Track>();
		trackInfo->index_ = VideoTrackType;
		trackInfo->codec_ = "h264";
		trackInfo->payloadType_ = 96;
		trackInfo->trackType_ = "video";
		trackInfo->samplerate_ = 90000;

		return trackInfo;
	});
}

void H264Track::onFrame(const FrameBuffer::Ptr& frame)
{
	if (_sps && _pps) {
		_hasReady = true;
		// return ;
	}

	if (!frame || frame->size() < 5) {
		return ;
	}

	if (frame->getNalType() == 7) {
        setSps(frame);
    } else if (frame->getNalType() == 8) {
        setPps(frame);
    }
}