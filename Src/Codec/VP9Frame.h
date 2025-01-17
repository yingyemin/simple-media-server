#ifndef VP9Frame_H
#define VP9Frame_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Common/Frame.h"

using namespace std;

enum VP9NalType
{
    VP9_KEYFRAME = 0,
    VP9_FRAME = 1,
};

class VP9Bitstream
{
public:
	VP9Bitstream() : m_data(NULL), m_len(0), m_idx(0), m_bits(0), m_byte(0), m_zeros(0) {};
	VP9Bitstream(void * data, int len) { Init(data, len); };
	void Init(void * data, int len)
	{
		m_data = (unsigned char*)data;
		m_len = len;
		m_idx = 0;
		m_bits = 0;
		m_byte = 0;
		m_zeros = 0;
	};
 
	uint8_t GetBYTE()
	{
		if (m_idx >= m_len)
			return 0;
		uint8_t b = m_data[m_idx++];
		// to avoid start-code emulation, a byte 0x03 is inserted
		// after any 00 00 pair. Discard that here.
		if (b == 0)
		{
			m_zeros++;
			if ((m_idx < m_len) && (m_zeros == 2) && (m_data[m_idx] == 0x03))
			{
 
				m_idx++;
				m_zeros = 0;
			}
		}
		else {
			m_zeros = 0;
		}
		return b;
	};
 
	uint32_t GetBit()
	{
		if (m_bits == 0)
		{
			m_byte = GetBYTE();
			m_bits = 8;
		}
		m_bits--;
		return (m_byte >> m_bits) & 0x1;
	};
 
	uint32_t GetWord(int bits)
	{
 
		uint32_t u = 0;
		while (bits > 0)
		{
			u <<= 1;
			u |= GetBit();
			bits--;
		}
		return u;
	};
 
 
private:
	unsigned char* m_data;
	int m_len;
	int m_idx;
	int m_bits;
	uint8_t m_byte;
	int m_zeros;
};

class VP9Frame : public FrameBuffer
{
public:
    using Ptr = shared_ptr<VP9Frame>;

    VP9Frame()
    {
        _codec = "vp9";
    }
    
    VP9Frame(const VP9Frame::Ptr& frame);

    bool isNewNalu() override;

    bool keyFrame() const override;

    bool metaFrame() const override
    {
        return keyFrame();
    }

    bool startFrame() const override
    {
        return keyFrame();
    }

    uint8_t getNalType() override
    {
        if (keyFrame()) {
            return VP9_KEYFRAME;
        } else {
            return VP9_FRAME;
        }
    }

    bool isNonPicNalu() override
    {
        return false;
    }

    static uint8_t getNalType(uint8_t* nalByte, int len);

    void split(const function<void(const FrameBuffer::Ptr& frame)>& cb) override;
    static FrameBuffer::Ptr createFrame(int startSize, int index, bool addStart);
    
    static void registerFrame();

public:
    bool _keyframe = false;
};


#endif //VP9Frame_H
