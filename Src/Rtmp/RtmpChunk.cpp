#include "RtmpChunk.h"
#include "Rtmp.h"
#include "Util/String.h"
#include "Log/Logger.h"

RtmpChunk::RtmpChunk()
{
	_state = PARSE_HEADER;
	_chunkStreamId = -1;
	_streamId = kDefaultStreamId;
}

RtmpChunk::~RtmpChunk()
{

}

int RtmpChunk::parse(const StreamBuffer::Ptr& buffer)
{
	int ret = 0;

	if (!buffer->size()) {
		return 0;
	}

    char* buf = buffer->data();
	uint32_t size = buffer->size();
    uint32_t bytesUsed = 0;

    if (_remainBuffer.size() > 0) {
        _remainBuffer.append(buf, size);
        buf = _remainBuffer.data();
        size = _remainBuffer.size();
    }

    do {
        if (_state == PARSE_HEADER) {
            ret = parseChunkHeader((uint8_t*)buf, size, bytesUsed);
        }
        else if (_state == PARSE_BODY)
        {
            ret = parseChunkBody((uint8_t*)buf, size, bytesUsed);
            if (ret > 0 && _chunkStreamId >= 0) {
                auto& rtmpMsg = _messages[_chunkStreamId];

                if (rtmpMsg.index == rtmpMsg.length) {
                    if (rtmpMsg.timestamp >= 0xffffff) {
						// logInfo << "rtmpMsg._timestamp: " << rtmpMsg.abs_timestamp;
                        rtmpMsg.abs_timestamp += rtmpMsg.extend_timestamp;
                    }
                    else {
						// logInfo << "rtmpMsg._timestamp: " << rtmpMsg._timestamp 
						// 			<< ", _chunkStreamId: " << _chunkStreamId
						// 			<< ", rtmpMsg.timestamp: " << rtmpMsg.timestamp;
                        rtmpMsg.abs_timestamp += rtmpMsg.timestamp;
                    }

                    if (_onRtmpChunk) {
                        _onRtmpChunk(rtmpMsg);
                    }
                    _chunkStreamId = -1;
                    // rtmpMsg.clear();
					rtmpMsg.index = 0;
					rtmpMsg.timestamp = 0;
					rtmpMsg.extend_timestamp = 0;
					rtmpMsg.payload = nullptr;
                }
            }
        }
    } while (ret > 0);

    // logInfo << "bytesUsed: " << bytesUsed << ", size: " << size;
    if (bytesUsed < size) {
        _remainBuffer.assign((char*)buf + bytesUsed, size - bytesUsed);
    } else if (_remainBuffer.size() > 0) {
        _remainBuffer.clear();
    }
	
    return ret;
}

int RtmpChunk::parseChunkHeader(uint8_t* buf, uint32_t size, uint32_t &bytesUsed)
{
    // logInfo << "parseChunkHeader, _remainBuffer.size(): " << _remainBuffer.size();
	// uint32_t bytes_used = 0;
	// uint8_t* buf = (uint8_t*)_remainBuffer.data();
	// uint32_t buf_size = _remainBuffer.size();

    // if (_remainBuffer.size() > 0) {
    //     _remainBuffer.append(buffer->data(), buf_size);
    //     buf = (uint8_t*)_remainBuffer.data();
    //     buf_size = _remainBuffer.size();
    //     _remainBuffer.clear();
    // }

	uint32_t headerBytesUsed = bytesUsed;

	uint8_t flags = buf[headerBytesUsed];
	headerBytesUsed += 1;

	uint8_t csid = flags & 0x3f; // chunk stream id
	if (csid == 0) { // csid [64, 319] 
		if (size < (headerBytesUsed + 2)) {
			// logInfo << "left size is < 2";
			return 0;
		}

		csid += buf[headerBytesUsed] + 64;
		headerBytesUsed += 1;
	}
	else if (csid == 1) { // csid [64, 65599]
		if (size < (3 + headerBytesUsed)) {
			// logInfo << "left size is < 3";
			return 0;
		}
		csid += buf[headerBytesUsed + 1] * 255 + buf[headerBytesUsed] + 64;
		headerBytesUsed += 2;
	}

	uint8_t fmt = (flags >> 6); // message header type
	if (fmt >= 4) {
		// logInfo << "fmt is >= 4";
		return -1;
	}

	uint32_t header_len = kChunkMessageHeaderLen[fmt]; // basic_header + message_header 
	if (size < (header_len + headerBytesUsed)) {
		// logInfo << "left size is < header_len";
		return 0;
	}

	RtmpMessageHeader header;
	memset(&header, 0, sizeof(RtmpMessageHeader));
	memcpy(&header, buf + headerBytesUsed, header_len);
	headerBytesUsed += header_len;

	auto& msg = _messages[csid];
	_chunkStreamId = msg.csid = csid;

	if (fmt == RTMP_CHUNK_TYPE_0 || fmt == RTMP_CHUNK_TYPE_1) {
		uint32_t length = readUint24BE((char*)header.length);
		if (msg.length != length || !msg.payload) {
			msg.length = length;
			// msg.payload.reset(new char[msg.length], std::default_delete<char[]>());
		}
		msg.index = 0;
		msg.type_id = header.type_id;
	}

	if (fmt == RTMP_CHUNK_TYPE_0) {
		msg.stream_id = readUint24LE((char*)header.stream_id);
	}

	uint32_t timestamp = readUint24BE((char*)header.timestamp);
	if (fmt == RTMP_CHUNK_TYPE_1 || fmt == RTMP_CHUNK_TYPE_2) {
		msg.laststep = timestamp;
	} else if (fmt == RTMP_CHUNK_TYPE_3) {
		timestamp = msg.laststep;
	}
	// logInfo << "header timestamp: " << timestamp 
	// 			<< ", header size: " << sizeof(header)
	// 			<< ", fmt: " << (int)fmt;
	uint32_t extend_timestamp = 0;
	if (timestamp >= 0xffffff || msg.timestamp >= 0xffffff) {
		if (size < (4 + headerBytesUsed)) {
			// logInfo << "left size is < 4";
			return 0;
		}
		extend_timestamp = readUint32BE((char*)buf + headerBytesUsed);
		headerBytesUsed += 4;
	}

	if (msg.index == 0) { // first chunk
		if (fmt == RTMP_CHUNK_TYPE_0) {
			// absolute timestamp 
			msg.abs_timestamp = 0;
			msg.timestamp = timestamp;
			msg.extend_timestamp = extend_timestamp;
		}
		else {
			// relative timestamp (timestamp delta)
			if (msg.timestamp >= 0xffffff) {
				msg.extend_timestamp += extend_timestamp;
			}
			else {
				msg.timestamp += timestamp;
			}
		}
	}

	if (!msg.payload) {
		logInfo << "create a buffer: " << msg.length;
		msg.payload = make_shared<StreamBuffer>(msg.length + 1);
	}

	_state = PARSE_BODY;
	// buffer.Retrieve(bytesUsed);
    // logInfo << "bytesUsed: " << bytesUsed << ", buf_size: " << buf_size;
    // if (bytesUsed < buf_size) {
    //     _remainBuffer.assign((char*)buf + bytesUsed, buf_size - bytesUsed);
    // } else {
    //     _remainBuffer.clear();
    // }
	bytesUsed = headerBytesUsed;

	return bytesUsed;
}

int RtmpChunk::parseChunkBody(uint8_t* buf, uint32_t size, uint32_t &bytesUsed)
{
    // logInfo << "parseChunkHeader, _remainBuffer.size(): " << _remainBuffer.size();
	// uint32_t bytes_used = 0;
	// uint8_t* buf = (uint8_t*)_remainBuffer.data();
	// uint32_t buf_size = _remainBuffer.size();

    // if (_remainBuffer.size() > 0) {
    //     _remainBuffer.append(buffer->data(), buf_size);
    //     buf = (uint8_t*)_remainBuffer.data();
    //     buf_size = _remainBuffer.size();
    //     _remainBuffer.clear();
    // }

	uint32_t bodyBytesUsed = bytesUsed;

	if (_chunkStreamId < 0) {
		logInfo << "_chunkStreamId < 0";
		return -1;
	}

	auto& msg = _messages[_chunkStreamId];
	uint32_t chunkSize = msg.length - msg.index;

	if (chunkSize > _inChunkSize) {
		chunkSize = _inChunkSize;
	}

	logInfo << "size: " << size;
	logInfo << "chunkSize: " << chunkSize;
	logInfo << "bodyBytesUsed: " << bodyBytesUsed;
	logInfo << "msg.index: " << msg.index;
	if (size < (chunkSize + bodyBytesUsed)) {
		return 0;
	}

	if (msg.index + chunkSize > msg.length) {
		return -1;
	}

	memcpy(msg.payload->data() + msg.index, buf + bodyBytesUsed, chunkSize);
	bodyBytesUsed += chunkSize;
	msg.index += chunkSize;
	
	// logInfo << "size: " << size;
	// logInfo << "chunkSize: " << chunkSize;
	// logInfo << "bodyBytesUsed: " << bodyBytesUsed;
	// logInfo << "msg.index: " << msg.index;

	if (msg.index >= msg.length ||
		msg.index%_inChunkSize == 0) {
		_state = PARSE_HEADER;
	}

	// buffer.Retrieve(bytes_used);
    // logInfo << "bytes_used: " << bytes_used << ", buf_size: " << buf_size;
    // if (buf_size > bytes_used) {
    //     _remainBuffer.assign((char*)buf + bytes_used, buf_size - bytes_used);
    // } else {
    //     _remainBuffer.clear();
    // }
	bytesUsed = bodyBytesUsed;
	return bytesUsed;
}

StreamBuffer::Ptr RtmpChunk::createBasicHeader(uint8_t fmt, uint32_t csid)
{
	int len = 0;
	StreamBuffer::Ptr buffer;

	if (csid >= 64 + 255) {
		buffer = make_shared<StreamBuffer>(4);
		char* buf = buffer->data();
		buf[len++] = (fmt << 6) | 1;
		buf[len++] = (csid - 64) & 0xFF;
		buf[len++] = ((csid - 64) >> 8) & 0xFF;
	}
	else if (csid >= 64) {
		buffer = make_shared<StreamBuffer>(3);
		char* buf = buffer->data();
		buf[len++] = (fmt << 6) | 0;
		buf[len++] = (csid - 64) & 0xFF;
	}
	else {
		buffer = make_shared<StreamBuffer>(2);
		char* buf = buffer->data();
		buf[len++] = (fmt << 6) | csid;
	}

	return buffer;
}

int RtmpChunk::createMessageHeader(uint8_t fmt, RtmpMessage& msg, uint64_t dts)
{
	int len = 0;

	StreamBuffer::Ptr buffer = make_shared<StreamBuffer>(12);
	auto buf = buffer->data();

	if (fmt <= 2) {
		if (dts < 0xffffff) {
			writeUint24BE((char*)buf, (uint32_t)dts);
		}
		else {
			writeUint24BE((char*)buf, 0xffffff);
		}
		len += 3;
	}

	if (fmt <= 1) {
		writeUint24BE((char*)buf + len, msg.length);
		len += 3;
		buf[len++] = msg.type_id;
	}

	if (fmt == 0) {
		writeUint32LE((char*)buf + len, msg.stream_id);
		len += 4;
	}

	buffer->substr(0, len);
	_socket->send(buffer, 0);
	return len;
}

int RtmpChunk::createChunk(uint32_t csid, RtmpMessage& msg)
{
	uint32_t payloadOffset = 0;

	uint32_t length = msg.length;
	uint64_t dts = msg.abs_timestamp;
	// if (msg.type_id == RTMP_VIDEO) {
	// 	_videoStampAdjust.inputStamp(dts, dts);
	// } else {
	// 	_audioStampAdjust.inputStamp(dts, dts, length);
	// }

	auto basicBuffer = createBasicHeader(0, csid); //first chunk
	_socket->send(basicBuffer, 0);

	createMessageHeader(0, msg, dts);

	StreamBuffer::Ptr extBuffer;
	if (dts >= 0xffffff) {
		extBuffer = make_shared<StreamBuffer>(5);
		auto buf = extBuffer->data();
		writeUint32BE((char*)buf, (uint32_t)dts);
		_socket->send(extBuffer, 0);
	}

	auto basic3Buffer = createBasicHeader(3, csid);

	while (length > 0)
	{
		// logInfo << "pkt len: " << length << ", _outChunkSize: " << _outChunkSize;
		if (length > _outChunkSize) {
			_socket->send(msg.payload, 0, payloadOffset, _outChunkSize);
			payloadOffset += _outChunkSize;
			length -= _outChunkSize;

			_socket->send(basic3Buffer, 0);
			if (dts >= 0xffffff) {
				_socket->send(extBuffer, 0);
			}
		}
		else {
			_socket->send(msg.payload, 1, payloadOffset, length);
			length = 0;
			break;
		}
	}

	return 0;
}

void RtmpChunk::setOnRtmpChunk(const function<void(const RtmpMessage msg)> cb)
{
    _onRtmpChunk = cb;
}