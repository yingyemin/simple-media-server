#ifndef FlvMuxerWithRtmp_H
#define FlvMuxerWithRtmp_H

#include <memory>
#include <string>

#include "RtmpMediaSource.h"

using namespace std;

class FlvMuxerWithRtmp : public enable_shared_from_this<FlvMuxerWithRtmp>
{
public:
	FlvMuxerWithRtmp(const UrlParser& urlParser, const EventLoop::Ptr& loop);
	virtual ~FlvMuxerWithRtmp();

	void start();
	void setOnWrite(const function<void(const char* data, int len)>& cb) {_onWrite = cb;}
	void setOnWrite(const function<void(const StreamBuffer::Ptr& buffer)>& cb) {_onWriteBuffer = cb;}
	void setOnDetach(const function<void()>& cb) {_onDetach = cb;}

	void setLocalIp(const string& ip) {_localIp = ip;}
	void setLocalPort(int port) {_localPort = port;}

	void setPeerIp(const string& ip) {_peerIp = ip;}
	void setPeerPort(int port) {_peerPort = port;}
	
	virtual bool ssPlaying()  { return _isPlaying; }
	virtual bool ssPlayer()  { return true; }

	virtual bool sendMediaData(uint8_t type, uint64_t timestamp, const StreamBuffer::Ptr& payload, uint32_t payload_size);
	virtual bool sendVideoData(uint64_t timestamp, const StreamBuffer::Ptr& payload, uint32_t payload_size);
	virtual bool sendAudioData(uint64_t timestamp, const StreamBuffer::Ptr& payload, uint32_t payload_size);

	void onError(const string& msg);

private:
	void onPlay();
	bool hasFlvHeader() const { return _hasFlvHeader; }
	void sendFlvHeader();
	int  sendFlvTag(uint8_t type, uint64_t timestamp, const StreamBuffer::Ptr& payload, uint32_t payload_size);
	void send(const char* data, int len);
	void send(const StreamBuffer::Ptr& buffer);

private:
	string _localIp;
	int _localPort = 0;
	string _peerIp;
	int _peerPort = 0;
	EventLoop::Ptr _loop;

	StreamBuffer::Ptr _avcSequenceHeader;
	StreamBuffer::Ptr _aacSequenceHeader;
	uint32_t _avcSequenceSeaderSize = 0;
	uint32_t _aacSequenceHeaderSize = 0;
    uint64_t _lastMuteId = 0;
	bool _hasKeyFrame = false;
	bool _hasFlvHeader = false;
	bool _isPlaying = false;
	bool _addMute = false;

	UrlParser _urlParser;
	RtmpMediaSource::Wptr _source;
	RtmpMediaSource::RingType::DataQueReaderT::Ptr _playReader;

	function<void(const char* data, int len)> _onWrite;
	function<void(const StreamBuffer::Ptr& buffer)> _onWriteBuffer;
	function<void()> _onDetach;
};

#endif
