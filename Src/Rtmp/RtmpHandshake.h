#ifndef RtmpHandshake_H
#define RtmpHandshake_H

#include "Net/Buffer.h"

class RtmpHandshake
{
public:
	enum State
	{
		HANDSHAKE_C0C1,
		HANDSHAKE_S0S1S2,
		HANDSHAKE_C2,
		HANDSHAKE_COMPLETE
	};

	RtmpHandshake(State state);
	virtual ~RtmpHandshake();

	int parse(const StreamBuffer::Ptr& buffer);

	int buildC0C1(char* buf, uint32_t buf_size);

	bool isCompleted() const
	{
		return _state == HANDSHAKE_COMPLETE;
	}

    void setOnHandshake(const function<void(const StreamBuffer::Ptr& buffer)>& cb);
    void onHandshake(const StreamBuffer::Ptr& buffer);

	void setOnRtmpChunk(const function<void(const StreamBuffer::Ptr& buffer)>& cb);
	void onRtmpChunk(const StreamBuffer::Ptr& buffer);

private:
	State _state;
	StringBuffer _remainBuffer;
    function<void(const StreamBuffer::Ptr& buffer)> _onHandshake;
	function<void(const StreamBuffer::Ptr& buffer)> _onRtmpChunk;
};

#endif