#include "FlvMuxerWithRtmp.h"
#include "Common/UrlParser.h"
#include "Log/Logger.h"
#include "Rtmp.h"
#include "RtmpMessage.h"
#include "Common/Define.h"
#include "Util/String.h"
#include "Hook/MediaHook.h"
#include "Codec/AacFrame.h"
#include "Codec/AacTrack.h"
#include "Common/Config.h"

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

	static bool enbaleAddMute = Config::instance()->getAndListen([](const json &config){
        enbaleAddMute = Config::instance()->get("Rtmp", "Server", "Server1", "enableAddMute");
    }, "Rtmp", "Server", "Server1", "enableAddMute");
    auto tracks = rtmpSrc->getTrackInfo();

    if (enbaleAddMute && tracks.size() == 1 && tracks.begin()->second->trackType_ == "video") {
        _addMute = true;
        logInfo << "send a aac header";
		auto payload = AacTrack::getMuteConfig();
		sendFlvTag(RTMP_AUDIO, 0, payload, payload->size());
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
				// uint8_t frame_type = (pkt->payload->data()[0] >> 4) & 0x0f;
				// uint8_t codec_id = pkt->payload->data()[0] & 0x0f;

				// FILE* fp = fopen("testflv.rtmp", "ab+");
				// fwrite(pkt->payload.get(), pkt->length, 1, fp);
				// fclose(fp);

				// logInfo << "frame_type : " << (int)frame_type << ", codec_id: " << (int)codec_id
				// 		<< "pkt->payload.get()[0]: " << (int)(pkt->payload.get()[0]);
				self->sendMediaData(pkt->type_id, pkt->abs_timestamp, pkt->payload, pkt->length);

				if (self->_addMute) {
                    // aac 一帧1024字节，采样率8000。一帧的时长，单位ms
                    static int scale = (1024 * 1000 / 8000);
                    int curMuteId = pkt->abs_timestamp / scale;
                    if (self->_lastMuteId != curMuteId) {
                        self->_lastMuteId = curMuteId;
                        auto buffer = AacFrame::getMuteForFlv();
						self->sendMediaData(RTMP_AUDIO, pkt->abs_timestamp, buffer, buffer->size());
                    }
                }
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
        auto source = make_shared<RtmpMediaSource>(self->_urlParser, nullptr, true);
        if (self->_urlParser.type_ == "enhanced") {
            source->setEnhanced(true);
        } else if (self->_urlParser.type_ == "fastPts") {
            source->setFastPts(true);
            return source;
        }

        return source;
    }, this);
}

bool FlvMuxerWithRtmp::sendMediaData(uint8_t type, uint64_t timestamp, const StreamBuffer::Ptr& payload, uint32_t payload_size)
{	 
	if (!payload || payload_size == 0) {
		return false;
	}

	_isPlaying = true;

	if (type == RTMP_VIDEO) {
		if (!_hasKeyFrame) {
			bool isEnhance = (payload->data()[0] >> 4) & 0b1000;
			uint8_t frame_type;// = (payload->data()[0] >> 4) & 0x0f;
			uint8_t codec_id;// = payload->data()[0] & 0x0f;
			if (isEnhance) {
				if (payload_size < 5) {
					return false;
				}
				frame_type = (payload->data()[0] >> 4) & 0b0111;
				if (readUint32BE((char*)payload->data() + 1) == fourccH265) {
					codec_id = RTMP_CODEC_ID_H265;
				} else if (readUint32BE((char*)payload->data() + 1) == fourccAV1) {
					codec_id = RTMP_CODEC_ID_AV1;
				} else if (readUint32BE((char*)payload->data() + 1) == fourccVP9) {
					codec_id = RTMP_CODEC_ID_VP9;
				}
			} else {
				frame_type = (payload->data()[0] >> 4) & 0x0f;
				codec_id = payload->data()[0] & 0x0f;
			}

			logInfo << "frame_type : " << (int)frame_type << ", codec_id: " << (int)codec_id
					<< ", timestamp: " << timestamp;

			if (frame_type == 1/* && (codec_id == RTMP_CODEC_ID_H264 || codec_id == RTMP_CODEC_ID_H265)*/) {
				_hasKeyFrame = true;
			}
			else {
				return true;
			}
		}
		// logInfo << "send video data: " << timestamp;
		sendVideoData(timestamp, payload, payload_size);
	}
	else if (type == RTMP_AUDIO) {
		if (!_hasKeyFrame && _avcSequenceSeaderSize > 0) {
			return true;
		}
		sendAudioData(timestamp, payload, payload_size);
	}

	return true;
}

bool FlvMuxerWithRtmp::sendVideoData(uint64_t timestamp, const StreamBuffer::Ptr& payload, uint32_t payload_size)
{
	sendFlvTag(RTMP_VIDEO, timestamp, payload, payload_size);
	return true;
}

bool FlvMuxerWithRtmp::sendAudioData(uint64_t timestamp, const StreamBuffer::Ptr& payload, uint32_t payload_size)
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

int FlvMuxerWithRtmp::sendFlvTag(uint8_t type, uint64_t timestamp, const StreamBuffer::Ptr& payload, uint32_t payload_size)
{
	if (payload_size == 0 || !payload) {
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
	if (payload->size() > payload_size) {
		payload->substr(0, payload_size);
	}
	send(payload);
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
	if (_onWrite && data) {
		_onWrite(data, len);
	}
}

void FlvMuxerWithRtmp::send(const StreamBuffer::Ptr& buffer)
{
	if (_onWriteBuffer && buffer) {
		_onWriteBuffer(buffer);
	}
}