#include "FlvMuxerWithRtmp.h"
#include "Common/UrlParser.h"
#include "Log/Logger.h"
#include "Rtmp.h"
#include "RtmpMessage.h"
#include "Common/Define.h"
#include "Util/String.h"
#include "Hook/MediaHook.h"

FlvMuxerWithRtmp::FlvMuxerWithRtmp(const UrlParser& urlParser, const EventLoop::Ptr& loop)
	:_loop(loop)
	,_urlParser(urlParser)
{
	logInfo << "FlvMuxerWithRtmp";
	_urlParser.protocol_ = PROTOCOL_RTMP;
}

FlvMuxerWithRtmp::~FlvMuxerWithRtmp()
{
	logInfo << "~FlvMuxerWithRtmp";
	auto rtmpSrc = _source.lock();
	if (rtmpSrc) {
		rtmpSrc->delConnection(this);
	}
	// if (_onDetach) {
	// 	_onDetach();
	// }

	if (_playReader) {
		PlayerInfo info;
        info.ip = _peerIp;
        info.port = _peerPort;
        info.protocol = PROTOCOL_HTTP_FLV;
        info.status = "off";
        info.type = _urlParser.type_;
        info.uri = _urlParser.path_;
        info.vhost = _urlParser.vhost_;

        MediaHook::instance()->onPlayer(info);
	}
}

void FlvMuxerWithRtmp::onPlay()
{
	weak_ptr<FlvMuxerWithRtmp> wSelf = shared_from_this();
	auto rtmpSrc = _source.lock();
	if (!rtmpSrc) {
		onError("source is empty");
		return ;
	}

	// rtmpSrc->addOnDetach(this, [wSelf](){
	// 	auto self = wSelf.lock();
	// 	if (!self) {
	// 		return ;
	// 	}
	// 	self->onError("source detach");
	// });
	logInfo << "Resetting and playing stream";

	sendFlvHeader();

	
	if (rtmpSrc->_avcHeader) {
		logInfo << "send a avc header";
		sendFlvTag(RTMP_VIDEO, 0, rtmpSrc->_avcHeader, rtmpSrc->_avcHeaderSize);
	}

	if (rtmpSrc->_aacHeader) {
		logInfo << "send a aac header";
		sendFlvTag(RTMP_AUDIO, 0, rtmpSrc->_aacHeader, rtmpSrc->_aacHeaderSize);
	}

	if (!_playReader) {
		logInfo << "set _playReader";
		_playReader = rtmpSrc->getRing()->attach(EventLoop::getCurrentLoop(), true);
		_playReader->setGetInfoCB([wSelf]() {
			auto self = wSelf.lock();
			ClientInfo ret;
			if (!self) {
				return ret;
			}
			ret.ip_ = self->_peerIp;
			ret.port_ = self->_peerPort;
			ret.protocol_ = PROTOCOL_HTTP_FLV;
			return ret;
		});
		_playReader->setDetachCB([wSelf]() {
			auto strong_self = wSelf.lock();
			if (!strong_self) {
				return;
			}
			// strong_self->shutdown(SockException(Err_shutdown, "rtsp ring buffer detached"));
			strong_self->onError("ring buffer detach");
		});
		logInfo << "setReadCB =================";
		_playReader->setReadCB([wSelf](const RtmpMediaSource::RingDataType &pack) {
			auto self = wSelf.lock();
			if (!self/* || pack->empty()*/) {
				return;
			}
			// logInfo << "send rtmp msg";
			auto pktList = *(pack.get());
			for (auto& pkt : pktList) {
				uint8_t frame_type = (pkt->payload.get()[0] >> 4) & 0x0f;
				uint8_t codec_id = pkt->payload.get()[0] & 0x0f;

				// FILE* fp = fopen("testflv.rtmp", "ab+");
				// fwrite(pkt->payload.get(), pkt->length, 1, fp);
				// fclose(fp);

				// logInfo << "frame_type : " << (int)frame_type << ", codec_id: " << (int)codec_id
				// 		<< "pkt->payload.get()[0]: " << (int)(pkt->payload.get()[0]);
				self->sendMediaData(pkt->type_id, pkt->abs_timestamp, pkt->payload, pkt->length);
			}
		});

		PlayerInfo info;
        info.ip = _peerIp;
        info.port = _peerPort;
        info.protocol = PROTOCOL_HTTP_FLV;
        info.status = "on";
        info.type = _urlParser.type_;
        info.uri = _urlParser.path_;
        info.vhost = _urlParser.vhost_;

        MediaHook::instance()->onPlayer(info);
	}
}

void FlvMuxerWithRtmp::start()
{
    weak_ptr<FlvMuxerWithRtmp> wSelf = shared_from_this();

	_urlParser.path_ = trimBack(_urlParser.path_, ".flv");
    MediaSource::getOrCreateAsync(_urlParser.path_, _urlParser.vhost_, _urlParser.protocol_, _urlParser.type_, 
    [wSelf](const MediaSource::Ptr &src){
        logInfo << "get a src";
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

		auto rtmpSrc = dynamic_pointer_cast<RtmpMediaSource>(src);
		if (!rtmpSrc) {
			self->onError("rtmp source is empty");
		}

        self->_source = rtmpSrc;

		self->_loop->async([wSelf](){
			auto self = wSelf.lock();
			if (!self) {
				return ;
			}

			self->onPlay();
		}, true);
    }, 
    [wSelf]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }
        return make_shared<RtmpMediaSource>(self->_urlParser, nullptr, true);
    }, this);
}

bool FlvMuxerWithRtmp::sendMediaData(uint8_t type, uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size)
{	 
	if (payload_size == 0) {
		return false;
	}

	is_playing_ = true;

	if (type == RTMP_VIDEO) {
		if (!has_key_frame_) {
			uint8_t frame_type = (payload.get()[0] >> 4) & 0x0f;
			uint8_t codec_id = payload.get()[0] & 0x0f;

			logInfo << "frame_type : " << (int)frame_type << ", codec_id: " << (int)codec_id
					<< ", timestamp: " << timestamp;

			if (frame_type == 1 && (codec_id == RTMP_CODEC_ID_H264 || codec_id == RTMP_CODEC_ID_H265)) {
				has_key_frame_ = true;
			}
			else {
				return true;
			}
		}
		// logInfo << "send video data: " << timestamp;
		sendVideoData(timestamp, payload, payload_size);
	}
	else if (type == RTMP_AUDIO) {
		if (!has_key_frame_ && avc_sequence_header_size_ > 0) {
			return true;
		}
		sendAudioData(timestamp, payload, payload_size);
	}

	return true;
}

bool FlvMuxerWithRtmp::sendVideoData(uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size)
{
	sendFlvTag(RTMP_VIDEO, timestamp, payload, payload_size);
	return true;
}

bool FlvMuxerWithRtmp::sendAudioData(uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size)
{
	sendFlvTag(RTMP_AUDIO, timestamp, payload, payload_size);
	return true;
}

void FlvMuxerWithRtmp::sendFlvHeader()
{
	auto rtmpSrc = _source.lock();

	char flv_header[9] = { 0x46, 0x4c, 0x56, 0x01, 0x00, 0x00, 0x00, 0x00, 0x09 };

	if (rtmpSrc->_avcHeaderSize > 0) {
		flv_header[4] |= 0x1;
	}

	if (rtmpSrc->_aacHeaderSize > 0) {
		flv_header[4] |= 0x4;
	}

	send(flv_header, 9);

	char previous_tag_size[4] = { 0x0, 0x0, 0x0, 0x0 };
	send(previous_tag_size, 4);

	auto metadata = rtmpSrc->getMetadata();
	if (metadata.size() > 0) {
		AmfEncoder amfEncoder; 
		amfEncoder.encodeString("onMetaData", 10);
		amfEncoder.encodeECMA(metadata);

		sendFlvTag(RTMP_NOTIFY, 0, amfEncoder.data(), amfEncoder.size());
	}
}

int FlvMuxerWithRtmp::sendFlvTag(uint8_t type, uint64_t timestamp, std::shared_ptr<char> payload, uint32_t payload_size)
{
	if (payload_size == 0) {
		return -1;
	}

	char tag_header[11] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	char previous_tag_size[4] = { 0x0, 0x0, 0x0, 0x0 };

	tag_header[0] = type;
	writeUint24BE(tag_header + 1, payload_size);
	tag_header[4] = (timestamp >> 16) & 0xff;
	tag_header[5] = (timestamp >> 8) & 0xff;
	tag_header[6] = timestamp & 0xff;
	tag_header[7] = (timestamp >> 24) & 0xff;

	writeUint32BE(previous_tag_size, payload_size + 11);

	send(tag_header, 11);
	send(payload.get(), payload_size);
	send(previous_tag_size, 4);
	return 0;
}

void FlvMuxerWithRtmp::onError(const string& msg)
{
	if (_onDetach) {
		_onDetach();
	}
}

void FlvMuxerWithRtmp::send(const char* data, int len)
{
	if (_onWrite) {
		_onWrite(data, len);
	}
}