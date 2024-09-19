#ifndef RtmpChunk_H
#define RtmpChunk_H

#include "RtmpMessage.h"
#include "Amf.h"
#include "Net/Buffer.h"
#include "Common/StampAdjust.h"
#include "Net/Socket.h"

#include <map>

// #pragma pack(1)

using namespace std;

class RtmpChunk
{
public:
	enum State
	{
		PARSE_HEADER,
		PARSE_BODY,
	};

	RtmpChunk();
	virtual ~RtmpChunk();

	int parse(const StreamBuffer::Ptr& in_buffer);

	int createChunk(uint32_t csid, RtmpMessage& rtmp_msg);

	void setInChunkSize(uint32_t inChunkSize)
	{ _inChunkSize = inChunkSize; }

	uint32_t getInChunkSize()
	{ return _inChunkSize; }

	void setOutChunkSize(uint32_t outChunkSize)
	{ _outChunkSize = outChunkSize; }

	void clear() 
	{ _messages.clear(); }

	int getStreamId() const
	{ return _streamId; }

    void setOnRtmpChunk(const function<void(const RtmpMessage msg)> cb);

	void setSocket(const Socket::Ptr& socket) {_socket = socket;}

private:
	int parseChunkHeader(uint8_t* buf, uint32_t buf_size, uint32_t &bytes_used);
	int parseChunkBody(uint8_t* buf, uint32_t buf_size, uint32_t &bytes_used);
	StreamBuffer::Ptr createBasicHeader(uint8_t fmt, uint32_t csid);
	int createMessageHeader(uint8_t fmt, RtmpMessage& rtmp_msg, uint64_t dts);

private:
	State _state;
	int _chunkStreamId = 0;
	int _streamId = 0;
	uint32_t _inChunkSize = 128;
	uint32_t _outChunkSize = 128;
	VideoStampAdjust _videoStampAdjust;
	AudioStampAdjust _audioStampAdjust;
    StringBuffer _remainBuffer;
	Socket::Ptr _socket;
	std::map<int, RtmpMessage> _messages;
    function<void(const RtmpMessage msg)> _onRtmpChunk;

	const int kDefaultStreamId = 1;
	const int kChunkMessageHeaderLen[4] = { 11, 7, 3, 0 };
};


#endif