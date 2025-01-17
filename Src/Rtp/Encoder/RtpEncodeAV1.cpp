#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpEncodeAV1.h"
#include "Logger.h"
#include "Util/String.h"
#include "Codec/AV1Frame.h"

using namespace std;

#define AV1_AGGREGATION_HEADER_Z 0x80 // set to 1 if the first OBU element is an OBU fragment that is a continuation of an OBU fragment from the previous packet, 0 otherwise.
#define AV1_AGGREGATION_HEADER_Y 0x40 // set to 1 if the last OBU element is an OBU fragment that will continue in the next packet, 0 otherwise.
#define AV1_AGGREGATION_HEADER_N 0x08 // set to 1 if the packet is the first packet of a coded video sequence, 0 otherwise. Note: if N equals 1 then Z must equal 0.

RtpEncodeAV1::RtpEncodeAV1(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{}

void RtpEncodeAV1::encode(const FrameBuffer::Ptr& frame)
{
    auto payload = (uint8_t *)frame->data() + frame->startSize();
    auto pts = frame->pts();
    auto size = frame->size() - frame->startSize();
    size_t index = 0;
    uint8_t obu_type;
    uint8_t offset = 0;
    uint64_t len;
    const uint8_t* ptr = nullptr;
    uint8_t aggregation = 0;

    while (index < size) {
        obu_type = (payload[index] >> 3) & 0x0F;
        if (payload[index] & 0x04) // obu_extension_flag
		{
			// http://aomedia.org/av1/specification/syntax/#obu-extension-header-syntax
			// temporal_id = (obu[1] >> 5) & 0x07;
			// spatial_id = (obu[1] >> 3) & 0x03;
			offset = 2;
		}
		else
		{
			offset = 1;
		}

        if (payload[index] & 0x02) // obu_has_size_field
		{
			ptr = leb128(payload + index + offset, (int)(size - index - offset), &len);
			if (ptr + len > payload + size)
				return ;
			len += ptr - payload - index;
		}
		else
		{
			len = size - index;
		}

		// 5. Packetization rules
		// The temporal delimiter OBU, if present, SHOULD be removed 
		// when transmitting, and MUST be ignored by receivers.
		if (OBU_TEMPORAL_DELIMITER == obu_type)
			continue;

		aggregation |= OBU_SEQUENCE_HEADER == obu_type ? AV1_AGGREGATION_HEADER_N : 0;

        encodeObu(payload + index, (size_t)len, aggregation, frame->startFrame(), pts);
    }
}

void RtpEncodeAV1::encodeObu(const uint8_t* payload, size_t size, uint8_t aggregation, bool gopStart, uint64_t pts)
{
    bool start = true;
    while (size > 0) {
        if (size <= _maxRtpSize) {
            makeRtp((char*)payload, size, start, true, gopStart, pts, aggregation);
            break;
        }
        makeRtp((char*)payload, _maxRtpSize, start, false, gopStart, pts, aggregation | AV1_AGGREGATION_HEADER_Y);
        payload += _maxRtpSize;
        size -= _maxRtpSize;
        start = false;

        aggregation = 0;
        aggregation |= AV1_AGGREGATION_HEADER_Z;
    }
    return ;
}

void RtpEncodeAV1::makeRtp(const char *data, size_t len, bool start, bool mark, bool gopStart, uint64_t pts, uint8_t aggregation)
{
    // rtp header 12
    // aggregation header 1
    int rtpLen = len + 12 + 1;
    if (len > 0x7F) {
        rtpLen += 2;
    } else {
        rtpLen += 1;
    }
    RtpPacket::Ptr rtp = RtpPacket::create(_trackInfo, rtpLen, pts, _ssrc, _lastSeq++, mark);
    auto payload = rtp->getPayload();
    auto end = payload + rtpLen - 12;
    auto ptr = payload + 1;
    

    ptr = leb128_write(len, ptr, len);
    memcpy(ptr, data, len);

    payload[0] = aggregation;

    onRtpPacket(rtp, gopStart);
}

void RtpEncodeAV1::setOnRtpPacket(const function<void(const RtpPacket::Ptr& packet, bool start)>& cb)
{
    _onRtpPacket = cb;
}

void RtpEncodeAV1::onRtpPacket(const RtpPacket::Ptr& rtp, bool start)
{
    if (_onRtpPacket) {
        _onRtpPacket(rtp, start);
    }
}