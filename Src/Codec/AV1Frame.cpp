#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "AV1Frame.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Config.h"

using namespace std;

static inline const uint8_t* leb128(const uint8_t* data, int bytes, uint64_t* v)
{
	int i;
	uint64_t b;

	b = 0x80;
	for (*v = i = 0; i * 7 < 64 && i < bytes && 0 != (b & 0x80); i++)
	{
		b = data[i];
		*v |= (b & 0x7F) << (i * 7);
	}
	return data + i;
}

int aom_av1_annexb_split(const uint8_t* data, size_t bytes, const function<int (const uint8_t* obu, size_t bytes)>& handler)
{
	int r;
    // 0:temporal_unit_size, 1:frame_size, 2:obu_size
	uint64_t n[3];
	const uint8_t* temporal, * frame, * obu;

	r = 0;
	for (temporal = data; temporal < data + bytes && 0 == r; temporal += n[0])
	{
		// temporal_unit_size
		temporal = leb128(temporal, (int)(data + bytes - temporal), &n[0]);
		if (temporal + n[0] > data + bytes)
			return -1;

		for (frame = temporal; frame < temporal + n[0] && 0 == r; frame += n[1])
		{
			// frame_unit_size
			frame = leb128(frame, (int)(temporal + n[0] - frame), &n[1]);
			if (frame + n[1] > temporal + n[0])
				return -1;

			for (obu = frame; obu < frame + n[1] && 0 == r; obu += n[2])
			{
                // obu_size
				obu = leb128(obu, (int)(frame + n[1] - obu), &n[2]);
				if (obu + n[2] > frame + n[1])
					return -1;

				r = handler(obu, (size_t)n[2]);
			}
		}
	}

	return r;
}

int aom_av1_obu_split(const uint8_t* data, size_t bytes, const function<int (const uint8_t* obu, size_t bytes)>& handler)
{
	int r;
	size_t i;
	size_t offset;
	uint64_t len;
	uint8_t obu_type;
	const uint8_t* ptr;

	for (i = r = 0; i < bytes && 0 == r; i += (size_t)len)
	{
		// http://aomedia.org/av1/specification/syntax/#obu-header-syntax
		obu_type = (data[i] >> 3) & 0x0F;
		if (data[i] & 0x04) // obu_extension_flag
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

		if (data[i] & 0x02) // obu_has_size_field
		{
			ptr = leb128(data + i + offset, (int)(bytes - i - offset), &len);
			if (ptr + len > data + bytes)
				return -1;
			len += ptr - data - i;
		}
		else
		{
			len = bytes - i;
		}

		r = handler(data + i, (size_t)len);
	}

	return r;
}


AV1Frame::AV1Frame()
{
    _codec = "av1";
    _trackType = 0; //VideoTrackType;
}

AV1Frame::AV1Frame(const AV1Frame::Ptr& frame)
{
    if (frame) {
        _codec = frame->_codec;
        _trackType = frame->_trackType;
        _pts = frame->_pts;
        _profile = frame->_profile;
        _index = frame->_index;
        _dts = frame->_dts;
    }
}

void AV1Frame::split(const function<void(const FrameBuffer::Ptr& frame)>& cb)
{
    if (!cb) {
        return ;
    }

    auto ptr = _buffer.data();
    size_t size = _buffer.size();

    aom_av1_annexb_split((uint8_t*)ptr, size, [this, cb](const uint8_t *obu, size_t bytes){
        AV1Frame::Ptr subFrame = make_shared<AV1Frame>(dynamic_pointer_cast<AV1Frame>(shared_from_this()));
        subFrame->_buffer.assign((char*)obu, bytes);

        // cb(start - prefix, next_start - start + prefix, prefix);
        cb(subFrame);

        return 0;
    });
}

bool AV1Frame::isNewNalu()
{
    return 1;
}

uint8_t AV1Frame::getObuType() const
{
    auto payload = _buffer.data();
    return (payload[0] >> 3) & 0x0F;
}

FrameBuffer::Ptr AV1Frame::createFrame(int startSize, int index, bool addStart)
{
    auto frame = make_shared<AV1Frame>();
        
    frame->_startSize = startSize;
    frame->_codec = "av1";
    frame->_index = index;
    frame->_trackType = 0;//VideoTrackType;

    return frame;
}

void AV1Frame::registerFrame()
{
	FrameBuffer::registerFrame("av1", AV1Frame::createFrame);
}