#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpDecodeAV1.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "Codec/AV1Frame.h"

using namespace std;

RtpDecodeAV1::RtpDecodeAV1(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{
    _frame = createFrame();
}

FrameBuffer::Ptr RtpDecodeAV1::createFrame()
{
    // auto frame = make_shared<AV1Frame>();
    auto frame = FrameBuffer::createFrame("av1", 0, _trackInfo->index_, 0);
    // frame->_startSize = 0;
    // frame->_codec = _trackInfo->codec_;
    // frame->_index = _trackInfo->index_;
    // frame->_trackType = VideoTrackType;

    return frame;
}

bool RtpDecodeAV1::isStartGop(const RtpPacket::Ptr& rtp)
{
    auto payload_size = rtp->getPayloadSize();
    if (payload_size <= 0) {
        // 无实际负载  [AUTO-TRANSLATED:305af48f]
        // No actual payload
        return false;
    }
    auto ptr = rtp->getPayload();

    // AV1 aggregation header
	/*
	0 1 2 3 4 5 6 7
	+-+-+-+-+-+-+-+-+
	|Z|Y| W |N|-|-|-|
	+-+-+-+-+-+-+-+-+
	*/
	uint8_t n = ptr[0] & 0x08; // MUST be set to 1 if the packet is the first packet of a coded video sequence, and MUST be set to 0 otherwise.


    if (n == 1) {
        return true;
    }

    return false;
}

/*
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|X|  CC   |M|     PT      |       sequence number         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           synchronization source (SSRC) identifier            |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
|            contributing source (CSRC) identifiers             |
|                             ....                              |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
|         0x100         |  0x0  |       extensions length       |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
|   0x1(ID)     |  hdr_length   |                               |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+                               |此处解析不考虑扩展头
|                                                               |跳过扩展头，从av1 aggr header开始解析
|          dependency descriptor (hdr_length #octets)           |
|                                                               |
|                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                               | Other rtp header extensions...|
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
| AV1 aggr hdr  |                                               |
+-+-+-+-+-+-+-+-+                                               |
|                                                               |
|                   Bytes 2..N of AV1 payload                   |
|                                                               |
|                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                               :    OPTIONAL RTP padding       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
void RtpDecodeAV1::decode(const RtpPacket::Ptr& rtp)
{
    auto payload_size = rtp->getPayloadSize();
    if (payload_size <= 0) {
        // 无实际负载  [AUTO-TRANSLATED:305af48f]
        // No actual payload
        return ;
    }
    auto ptr = rtp->getPayload();
    auto stamp = rtp->getStampMS();
    auto seq = rtp->getSeq();
    auto pend = ptr + payload_size;

    if (stamp != _lastStamp && !_firstRtp) {
        onFrame(_frame);
    }
    // AV1 aggregation header
	/*
	0 1 2 3 4 5 6 7
	+-+-+-+-+-+-+-+-+
	|Z|Y| W |N|-|-|-|
	+-+-+-+-+-+-+-+-+
	*/
	uint8_t z = ptr[0] & 0x80; // MUST be set to 1 if the first OBU element is an OBU fragment that is a continuation of an OBU fragment from the previous packet, and MUST be set to 0 otherwise.
	uint8_t y = ptr[0] & 0x40; // MUST be set to 1 if the last OBU element is an OBU fragment that will continue in the next packet, and MUST be set to 0 otherwise.
	uint8_t w = (ptr[0] & 0x30) >> 4; // two bit field that describes the number of OBU elements in the packet. This field MUST be set equal to 0 or equal to the number of OBU elements contained in the packet. If set to 0, each OBU element MUST be preceded by a length field.
	uint8_t n = ptr[0] & 0x08; // MUST be set to 1 if the packet is the first packet of a coded video sequence, and MUST be set to 0 otherwise.
    ptr++;

    if (n == 1 && z != 0) {
        logWarn << "invalid av1 rtp packet";
        // 暂时不退出，以便测试
        // return ;
    }

    if (z == 0 && n == 1) {
        onFrame(_frame);
    }

    uint64_t obuSize = 0; 
    for (int i = 1; ptr < pend; ptr += obuSize, i++)
	{
		if (i < w || 0 == w)
		{
			ptr = leb128(ptr, pend - ptr, &obuSize);
		}
		else
		{
			obuSize = pend - ptr;
		}

		// skip fragment frame OBU size
		if (ptr + obuSize > pend)
		{
			//assert(0);
			//unpacker->size = 0;
			logWarn << "丢包导致解析错误";
			//unpacker->flags |= RTP_PAYLOAD_FLAG_PACKET_LOST;
			return ; // invalid packet
		}

        _frame->_pts = stamp;
        _frame->_buffer->append((char *) ptr, obuSize);
	}

    if (rtp->getHeader()->mark) {
        onFrame(_frame);
    }
    _lastStamp = stamp;
    _firstRtp = false;
}

void RtpDecodeAV1::setOnDecode(const function<void(const FrameBuffer::Ptr& frame)> cb)
{
    _onFrame = cb;
}

void RtpDecodeAV1::onFrame(const FrameBuffer::Ptr& frame)
{
    if (frame->size() > 0) {
        frame->_dts = frame->_pts;
        frame->_index = _trackInfo->index_;
        if (_onFrame) {
            _onFrame(frame);
        }
    }
    
    // FILE* fp = fopen("test1.aac", "ab+");
    // fwrite(frame->data(), 1, frame->size(), fp);
    // fclose(fp);

    _frame = createFrame();
}