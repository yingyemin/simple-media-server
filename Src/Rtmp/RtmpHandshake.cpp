#include "RtmpHandshake.h"
#include "Log/Logger.h"
#include "Rtmp.h"

#include <cstdint>
#include <random>

RtmpHandshake::RtmpHandshake(State state)
{
	_state = state;
}

RtmpHandshake::~RtmpHandshake()
{

}

int RtmpHandshake::parse(const StreamBuffer::Ptr& buffer)
{
	if (!buffer) {
		return 0;
	}
	uint8_t *buf = (uint8_t*)buffer->data();
	uint32_t buf_size = buffer->size();
	uint32_t pos = 0;
	uint32_t res_size = 0;
	std::random_device rd;

    if (_remainBuffer.size() > 0) {
        _remainBuffer.append((char*)buf, buf_size);
        buf = (uint8_t*)_remainBuffer.data();
        buf_size = _remainBuffer.size();
    }

	do {
		if (_state == HANDSHAKE_S0S1S2) {
			if (buf_size < (1 + 1536 + 1536)) { //S0S1S2		
				break;
			}

			if (buf[0] != RTMP_VERSION) {
				logError << "unsupported rtmp version " << buf[0];
				return -1;
			}

			pos += 1 + 1536 + 1536;
			res_size = 1536;
			// memcpy(res_buf, buf + 1, 1536); //C2
			auto resBuffer = StreamBuffer::create();
			resBuffer->assign((char*)buf + 1, 1536); //C2
			onHandshake(resBuffer);
			_state = HANDSHAKE_COMPLETE;
		}
		else if (_state == HANDSHAKE_C0C1)
		{
			logDebug << "get a c0c1: " << buf_size;
			if (buf_size < 1537) { //c0c1
				break;
			}
			else
			{
				if (buf[0] != RTMP_VERSION) {
					return -1;
				}

				pos += 1537;
				res_size = 1 + 1536 + 1536;
				auto resBuffer = StreamBuffer::create();
				resBuffer->setCapacity(res_size + 1);
				auto resBuf = resBuffer->data();
				memset(resBuf, 0, 1537); //S0 S1 S2  
				resBuf[0] = RTMP_VERSION;

				char *p = resBuf; p += 9;
				for (int i = 0; i < 1528; i++) {
					*p++ = rd();
				}
				memcpy(p, buf + 1, 1536);
				onHandshake(resBuffer);
				_state = HANDSHAKE_C2;
			}
		}
		else if (_state == HANDSHAKE_C2)
		{
			logDebug << "get a c2: " << buf_size;
			if (buf_size < 1536) { //c2
				break;
			}
			else {
				pos = 1536;
				_state = HANDSHAKE_COMPLETE;
			}
		}
		else {
			return -1;
		}
	} while (0);

	// buffer.Retrieve(pos);
    if (pos < buf_size) {
        _remainBuffer.append((char*)buf + pos, buf_size - pos);
        if (_state == HANDSHAKE_COMPLETE) {
            auto resBuffer = StreamBuffer::create();
            resBuffer->assign(_remainBuffer.data(), _remainBuffer.size());
            onRtmpChunk(resBuffer);
            _remainBuffer.clear();
        }
    } else {
        _remainBuffer.clear();
    }
	return res_size;
}

int RtmpHandshake::buildC0C1(char* buf, uint32_t buf_size)
{
	if (!buf || buf_size < 1537) {
		return 0;
	}
	uint32_t size = 1 + 1536; //COC1  
	memset(buf, 0, 1537);
	buf[0] = RTMP_VERSION;

	std::random_device rd;
	uint8_t *p = (uint8_t *)buf; p += 9;
	for (int i = 0; i < 1528; i++) {
		*p++ = rd();
	}

	return size;
}

void RtmpHandshake::setOnHandshake(const std::function<void(const StreamBuffer::Ptr& buffer)>& cb)
{
    _onHandshake = cb;
}

void RtmpHandshake::onHandshake(const StreamBuffer::Ptr& buffer)
{
    if (_onHandshake) {
        _onHandshake(buffer);
    }
}

void RtmpHandshake::setOnRtmpChunk(const std::function<void(const StreamBuffer::Ptr& buffer)>& cb)
{
    _onRtmpChunk = cb;
}

void RtmpHandshake::onRtmpChunk(const StreamBuffer::Ptr& buffer)
{
    if (_onRtmpChunk && buffer) {
        _onRtmpChunk(buffer);
    }
}