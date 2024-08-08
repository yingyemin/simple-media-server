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
	void setOnDetach(const function<void()>& cb) {_onDetach = cb;}

	void setLocalIp(const string& ip) {_localIp = ip;}
	void setLocalPort(int port) {_localPort = port;}
	
	virtual bool ssPlaying()  { return is_playing_; }
	virtual bool ssPlayer()  { return true; }

	virtual bool sendMediaData(uint8_t type, uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size);
	virtual bool sendVideoData(uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size);
	virtual bool sendAudioData(uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size);

	void onError(const string& msg);

private:
	void onPlay();
	bool hasFlvHeader() const { return has_flv_header_; }
	void sendFlvHeader();
	int  sendFlvTag(uint8_t type, uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size);
	void send(const char* data, int len);

private:
	string _localIp;
	int _localPort = 0;
	EventLoop::Ptr _loop;

	std::shared_ptr<char> avc_sequence_header_;
	std::shared_ptr<char> aac_sequence_header_;
	uint32_t avc_sequence_header_size_ = 0;
	uint32_t aac_sequence_header_size_ = 0;
	bool has_key_frame_ = false;
	bool has_flv_header_ = false;
	bool is_playing_ = false;

	UrlParser _urlParser;
	RtmpMediaSource::Wptr _source;
	RtmpMediaSource::RingType::DataQueReaderT::Ptr _playReader;

	function<void(const char* data, int len)> _onWrite;
	function<void()> _onDetach;
};

#endif
