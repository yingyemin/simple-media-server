#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpDecodeVp9.h"
#include "Logger.h"
#include "Util/String.h"
#include "Codec/VP9Frame.h"

using namespace std;

RtpDecodeVP9::RtpDecodeVP9(const shared_ptr<TrackInfo>& trackInfo)
    :_trackInfo(trackInfo)
{
    _frame = createFrame();
}

FrameBuffer::Ptr RtpDecodeVP9::createFrame()
{
    // auto frame = make_shared<VP9Frame>();
    auto frame = FrameBuffer::createFrame("vp9", 0, _trackInfo->index_, 0);
    // frame->_startSize = 0;
    // frame->_codec = _trackInfo->codec_;
    // frame->_index = _trackInfo->index_;
    // frame->_trackType = VideoTrackType;

    return frame;
}

bool RtpDecodeVP9::isStartGop(const RtpPacket::Ptr& rtp)
{
    auto payload_size = rtp->getPayloadSize();
    if (payload_size <= 0) {
        // 无实际负载  [AUTO-TRANSLATED:305af48f]
        // No actual payload
        return false;
    }
    auto ptr = rtp->getPayload();
    auto stamp = rtp->getStampMS();
    auto seq = rtp->getSeq();
    auto pend = ptr + payload_size;
    // VP9 payload descriptor
	/*
	 0 1 2 3 4 5 6 7
	+-+-+-+-+-+-+-+-+
	|I|P|L|F|B|E|V|-| (REQUIRED)
	+-+-+-+-+-+-+-+-+
	*/
	uint8_t pictureid_present = ptr[0] & 0x80;
	uint8_t inter_picture_predicted_layer_frame = ptr[0] & 0x40;
	uint8_t layer_indices_preset = ptr[0] & 0x20;
	uint8_t flex_mode = ptr[0] & 0x10;
	uint8_t start_of_layer_frame = ptr[0] & 0x80;
	//end_of_layer_frame = ptr[0] & 0x04;
	uint8_t scalability_struct_data_present = ptr[0] & 0x02;
	ptr++;

	if (pictureid_present && ptr < pend)
	{
		//    +-+-+-+-+-+-+-+-+
		// I: |M|  PICTURE ID | (RECOMMENDED)
		//    +-+-+-+-+-+-+-+-+
		// M: |  EXTENDED PID | (RECOMMENDED)
		//    +-+-+-+-+-+-+-+-+

		//uint16_t picture_id;
		//picture_id = ptr[0] & 0x7F;
		if ((ptr[0] & 0x80) && ptr + 1 < pend)
		{
			//picture_id = (ptr[0] << 8) | ptr[1];
			ptr++;
		}
		ptr++;
	}

	if (layer_indices_preset && ptr < pend)
	{
		//	  +-+-+-+-+-+-+-+-+
		// L: | T | U | S | D | (CONDITIONALLY RECOMMENDED)
		//    +-+-+-+-+-+-+-+-+
		//    |   TL0PICIDX   | (CONDITIONALLY REQUIRED)
		//    +-+-+-+-+-+-+-+-+
		
		// ignore Layer indices
		if (0 == flex_mode)
			ptr++; // TL0PICIDX
		ptr++;
	}

	if (inter_picture_predicted_layer_frame && flex_mode && ptr < pend)
	{
		//      +-+-+-+-+-+-+-+-+							-\
		// P,F: |    P_DIFF   |N| (CONDITIONALLY REQUIRED)  - up to 3 times
		//      +-+-+-+-+-+-+-+-+							-/

		// ignore Reference indices
		if (ptr[0] & 0x01)
		{
			if ((ptr[1] & 0x01) && ptr + 1 < pend)
				ptr++;
			ptr++;
		}
		ptr++;
	}

	if (scalability_struct_data_present && ptr < pend)
	{
		/*
			+-+-+-+-+-+-+-+-+
		V:	| N_S |Y|G|-|-|-|
			+-+-+-+-+-+-+-+-+			 -\
		Y:	|     WIDTH		| (OPTIONAL) .
			+				+			 .
			|				| (OPTIONAL) .
			+-+-+-+-+-+-+-+-+			 . - N_S + 1 times
			|     HEIGHT	| (OPTIONAL) .
			+				+			 .
			|				| (OPTIONAL) .
			+-+-+-+-+-+-+-+-+			 -/				-\
		G:	|      N_G      | (OPTIONAL)
			+-+-+-+-+-+-+-+-+							-\
		N_G:|  T  |U| R |-|-| (OPTIONAL)				.
			+-+-+-+-+-+-+-+-+			 -\				. - N_G times
			|     P_DIFF    | (OPTIONAL) .  - R times	.
			+-+-+-+-+-+-+-+-+			 -/				-/
		*/
		uint8_t N_S, Y, G;
		N_S = ((ptr[0] >> 5) & 0x07) + 1;
		Y = ptr[0] & 0x10;
		G = ptr[0] & 0x80;
		ptr++;

		if (Y)
		{
			ptr += N_S * 4;
		}

		if (G && ptr < pend)
		{
			uint8_t i;
			uint8_t N_G = ptr[0];
			ptr++;

			for (i = 0; i < N_G && ptr < pend; i++)
			{
				uint8_t j;
				uint8_t R;

				R = (ptr[0] >> 2) & 0x03;
				ptr++;

				for (j = 0; j < R && ptr < pend; j++)
				{
					// ignore P_DIFF
					ptr++;
				}
			}
		}
	}

	if (ptr >= pend)
	{
		// assert(0);
		//helper->size = 0;
		// helper->lost = 1;
		//helper->flags |= RTP_PAYLOAD_FLAG_PACKET_LOST;
		return false; // invalid packet
	}

	if (start_of_layer_frame)
	{
		// new frame begin
		uint8_t frameType = VP9Frame::getNalType(ptr, pend - ptr);
        if (frameType == VP9_KEYFRAME)
		{
            return true;
        }
	}

    return false;
}

/*
0               1               2               3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|X|   CC  |M|      PT     |         sequence number       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|            synchronization source (SSRC) identifier           |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
|             contributing source (CSRC) identifiers            |
|                              ....                             |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
|             VP9 payload descriptor (integer #octets)          |
:                                                               :
|                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                               :  VP9 pyld hdr |               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+               |
|                                                               |
+                                                               |
:                    Bytes 2..N of VP9 payload                  :
|                                                               |
|                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                               :      OPTIONAL RTP padding     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
void RtpDecodeVP9::decode(const RtpPacket::Ptr& rtp)
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
    // VP9 payload descriptor
	/*
	 0 1 2 3 4 5 6 7
	+-+-+-+-+-+-+-+-+
	|I|P|L|F|B|E|V|-| (REQUIRED)
	+-+-+-+-+-+-+-+-+
	*/
	uint8_t pictureid_present = ptr[0] & 0x80;
	uint8_t inter_picture_predicted_layer_frame = ptr[0] & 0x40;
	uint8_t layer_indices_preset = ptr[0] & 0x20;
	uint8_t flex_mode = ptr[0] & 0x10;
	uint8_t start_of_layer_frame = ptr[0] & 0x80;
	//end_of_layer_frame = ptr[0] & 0x04;
	uint8_t scalability_struct_data_present = ptr[0] & 0x02;
	ptr++;

	if (pictureid_present && ptr < pend)
	{
		//    +-+-+-+-+-+-+-+-+
		// I: |M|  PICTURE ID | (RECOMMENDED)
		//    +-+-+-+-+-+-+-+-+
		// M: |  EXTENDED PID | (RECOMMENDED)
		//    +-+-+-+-+-+-+-+-+

		//uint16_t picture_id;
		//picture_id = ptr[0] & 0x7F;
		if ((ptr[0] & 0x80) && ptr + 1 < pend)
		{
			//picture_id = (ptr[0] << 8) | ptr[1];
			ptr++;
		}
		ptr++;
	}

	if (layer_indices_preset && ptr < pend)
	{
		//	  +-+-+-+-+-+-+-+-+
		// L: | T | U | S | D | (CONDITIONALLY RECOMMENDED)
		//    +-+-+-+-+-+-+-+-+
		//    |   TL0PICIDX   | (CONDITIONALLY REQUIRED)
		//    +-+-+-+-+-+-+-+-+
		
		// ignore Layer indices
		if (0 == flex_mode)
			ptr++; // TL0PICIDX
		ptr++;
	}

	if (inter_picture_predicted_layer_frame && flex_mode && ptr < pend)
	{
		//      +-+-+-+-+-+-+-+-+							-\
		// P,F: |    P_DIFF   |N| (CONDITIONALLY REQUIRED)  - up to 3 times
		//      +-+-+-+-+-+-+-+-+							-/

		// ignore Reference indices
		if (ptr[0] & 0x01)
		{
			if ((ptr[1] & 0x01) && ptr + 1 < pend)
				ptr++;
			ptr++;
		}
		ptr++;
	}

	if (scalability_struct_data_present && ptr < pend)
	{
		/*
			+-+-+-+-+-+-+-+-+
		V:	| N_S |Y|G|-|-|-|
			+-+-+-+-+-+-+-+-+			 -\
		Y:	|     WIDTH		| (OPTIONAL) .
			+				+			 .
			|				| (OPTIONAL) .
			+-+-+-+-+-+-+-+-+			 . - N_S + 1 times
			|     HEIGHT	| (OPTIONAL) .
			+				+			 .
			|				| (OPTIONAL) .
			+-+-+-+-+-+-+-+-+			 -/				-\
		G:	|      N_G      | (OPTIONAL)
			+-+-+-+-+-+-+-+-+							-\
		N_G:|  T  |U| R |-|-| (OPTIONAL)				.
			+-+-+-+-+-+-+-+-+			 -\				. - N_G times
			|     P_DIFF    | (OPTIONAL) .  - R times	.
			+-+-+-+-+-+-+-+-+			 -/				-/
		*/
		uint8_t N_S, Y, G;
		N_S = ((ptr[0] >> 5) & 0x07) + 1;
		Y = ptr[0] & 0x10;
		G = ptr[0] & 0x80;
		ptr++;

		if (Y)
		{
			ptr += N_S * 4;
		}

		if (G && ptr < pend)
		{
			uint8_t i;
			uint8_t N_G = ptr[0];
			ptr++;

			for (i = 0; i < N_G && ptr < pend; i++)
			{
				uint8_t j;
				uint8_t R;

				R = (ptr[0] >> 2) & 0x03;
				ptr++;

				for (j = 0; j < R && ptr < pend; j++)
				{
					// ignore P_DIFF
					ptr++;
				}
			}
		}
	}

	if (ptr >= pend)
	{
		// assert(0);
		//helper->size = 0;
		// helper->lost = 1;
		//helper->flags |= RTP_PAYLOAD_FLAG_PACKET_LOST;
		return ; // invalid packet
	}

	if (start_of_layer_frame)
	{
		// new frame begin
		onFrame(_frame);
	}

    _frame->_pts = stamp;
    _frame->_buffer.append((char *) ptr, pend - ptr);

    if (rtp->getHeader()->mark) {
        onFrame(_frame);
    }
}

void RtpDecodeVP9::setOnDecode(const function<void(const FrameBuffer::Ptr& frame)> cb)
{
    _onFrame = cb;
}

void RtpDecodeVP9::onFrame(const FrameBuffer::Ptr& frame)
{
    // TODO aac_cfg
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