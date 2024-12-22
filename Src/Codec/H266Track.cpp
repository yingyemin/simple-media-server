#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <cmath>
#include<string.h>

#include "H266Track.h"
#include "H266Frame.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/Base64.h"

using namespace std;
 

static void de_emulation_prevention(unsigned char* buf,unsigned int* buf_size)
{  
	if (!buf) {
		return ;
	}
    int i=0,j=0;  
    unsigned char* tmp_ptr=NULL;  
    unsigned int tmp_buf_size=0;  
    int val=0;  
  
    tmp_ptr=buf;  
    tmp_buf_size=*buf_size;  
    for(i=0;i<(tmp_buf_size-2);i++)  
    {  
        //check for 0x000003  
        val=(tmp_ptr[i]^0x00) +(tmp_ptr[i+1]^0x00)+(tmp_ptr[i+2]^0x03);  
        if(val==0)  
        {  
            //kick out 0x03  
            for(j=i+2;j<tmp_buf_size-1;j++)  
                tmp_ptr[j]=tmp_ptr[j+1];  
  
            //and so we should devrease bufsize  
            (*buf_size)--;  
        }  
    }  
}  

H266Track::H266Track()
{

}

string H266Track::getSdp()
{
    stringstream ss;
    ss << "m=video 0 RTP/AVP " << payloadType_ << "\r\n"
    //    << "b=AS:" << bitrate << "\r\n"
       << "a=rtpmap:" << payloadType_ << " H266/" << samplerate_ << "\r\n"
       << "a=fmtp:" << payloadType_;
	if (_vps && _sps && _pps) {
		auto vps = string(_vps->data() + _vps->startSize(), _vps->size() - _vps->startSize());
		auto sps = string(_sps->data() + _sps->startSize(), _sps->size() - _sps->startSize());
		auto pps = string(_pps->data() + _pps->startSize(), _pps->size() - _pps->startSize());

		ss << " sprop-vps=" << Base64::encode(vps) << "; "
		   << "sprop-sps=" << Base64::encode(sps) << "; "
		   << "sprop-pps=" << Base64::encode(pps) << "\r\n";
	} else {
		ss << "\r\n";
	}

	ss << "a=control:trackID=" << index_ << "\r\n";

	return ss.str();
}

void H266Track::getWidthAndHeight(int& width, int& height, int& fps)
{
    // if (_width && _height) {
    //     width = _width;
    //     height = _height;
    // }
	
	// if (!_sps || !_vps || !_pps) {
	// 	logError << "no sps/pps/vps";
	// 	return ;
	// }

    // vc_params_t params = {0};
    // ParseSequenceParameterSet((uint8_t*)(_sps->data() + _sps->startSize()), _sps->size() - _sps->startSize(), params);
    // _width = params.width;
    // _height = params.height;
    // width = _width;
    // height = _height;
}

string H266Track::getConfig()
{
    if (!_vps || !_sps || !_pps) {
        return "";
    }

    string config;

    auto spsSize = _sps->startSize();
    auto vpsSize = _vps->startSize();
    auto ppsSize = _pps->startSize();

    auto vpsBuffer = _vps->_buffer;
    int vpsLen = _vps->size() - vpsSize;
    auto spsBuffer = _sps->_buffer;
    int spsLen = _sps->size() - spsSize;
    auto ppsBuffer = _pps->_buffer;
    int ppsLen = _pps->size() - ppsSize;

    config.resize(17  + vpsLen + spsLen + ppsLen);
    auto data = (char*)config.data();

    *data++ = 0b11111 << 3 | 0b11 < 1 | 0b0;
	// 1 byte
    // 默认vps，sps，pps 各一个
    *data++ = 3; //numOfArrays
    //2 byte

    *data++ = 1 << 7 | H266NalType::H266_VPS & 0x1f; //array_completeness(1),reserved(2),NAL_unit_type(5)
	// 3 byte

    // 一个vps
    *data++ = 0; //numNalus
    *data++ = 1;
	// 5 byte

    *data++ = (uint8_t)(vpsLen >> 8); //sequence parameter set length high 8 bits
    *data++ = (uint8_t)(vpsLen); //sequence parameter set  length low 8 bits
	// 7 byte

    memcpy(data, vpsBuffer.data() + vpsSize, vpsLen); //H264 sequence parameter set
    data += vpsLen; 
    // 7 + vpsLen

    *data++ = 1 << 7 | H266NalType::H266_SPS & 0x3f;
	// 8 byte + vpsLen

    // 一个sps
    *data++ = 0;
    *data++ = 1;
	// 10 byte + vpsLen

    *data++ = (uint8_t)(spsLen >> 8); //sequence parameter set length high 8 bits
    *data++ = (uint8_t)(spsLen); //sequence parameter set  length low 8 bits
	// 12 byte + vpsLen

    memcpy(data, spsBuffer.data() + spsSize, spsLen); //H264 sequence parameter set
    data += spsLen; 
    // 12 + vpsLen + spsLen

    *data++ = 1 << 7 | H266NalType::H266_PPS & 0x3f;
	// 13 + vpsLen + spsLen

    // 一个pps
    *data++ = 0;
    *data++ = 1;
	// 15 + vpsLen + spsLen

    *data++ = (uint8_t)(ppsLen >> 8); //sequence parameter set length high 8 bits
    *data++ = (uint8_t)(ppsLen); //sequence parameter set  length low 8 bits
	// 17 + vpsLen + spsLen

    memcpy(data, ppsBuffer.data() + ppsSize, ppsLen); //H264 sequence parameter set
    data += ppsLen; 
    // 17  + vpsLen + spsLen + ppsLen

    return config;
}

bool H266Track::isBFrame(unsigned char* data, int size)
{
	if (!data || !_vps || !_sps || !_pps) {
		return false;
	}
	// de_emulation_prevention(data, (unsigned int *)&size);

	

	return false;
}

H266Track::Ptr H266Track::createTrack(int index, int payloadType, int samplerate)
{
    auto trackInfo = make_shared<H266Track>();
    trackInfo->index_ = index;
    trackInfo->codec_ = "h266";
    trackInfo->payloadType_ = payloadType;
    trackInfo->trackType_ = "video";
    trackInfo->samplerate_ = samplerate;

    return trackInfo;
}

void H266Track::registerTrackInfo()
{
	TrackInfo::registerTrackInfo("h266", [](int index, int payloadType, int samplerate){
		auto trackInfo = make_shared<H266Track>();
		trackInfo->index_ = VideoTrackType;
		trackInfo->codec_ = "h266";
		trackInfo->payloadType_ = 96;
		trackInfo->trackType_ = "video";
		trackInfo->samplerate_ = 90000;

		return trackInfo;
	});
}

void H266Track::onFrame(const FrameBuffer::Ptr& frame)
{
	if (_sps && _pps && _vps) {
		_hasReady = true;
		return ;
	}

	if (!frame || frame->size() < 5) {
		return ;
	}

	if (frame->getNalType() == H266_VPS) {
        setVps(frame);
    } else if (frame->getNalType() == H266_SPS) {
        setPps(frame);
    } else if (frame->getNalType() == H266_PPS) {
        setPps(frame);
    }
}