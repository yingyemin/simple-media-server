#include "RtmpClient.h"
#include "Rtmp.h"
#include "Common/Define.h"
#include "Util/String.h"
#include "Hook/MediaHook.h"

// mutex RtmpClient::_mapMtx;
// unordered_map<string, RtmpClient::Ptr> RtmpClient::_mapRtmpClient;

RtmpClient::RtmpClient(MediaClientType type, const string& appName, const string& streamName)
    :TcpClient(EventLoop::getCurrentLoop())
    ,_type(type)
    ,_localAppName(appName)
    ,_localStreamName(streamName)
{
    _localUrlParser.path_ = "/" + _localAppName + "/" + _localStreamName;
    _localUrlParser.protocol_ = PROTOCOL_RTMP;
    _localUrlParser.type_ = DEFAULT_TYPE;
    _localUrlParser.vhost_ = DEFAULT_VHOST;
}

RtmpClient::~RtmpClient()
{
    logInfo << "~RtmpClient";
    auto rtmpSrc = _source.lock();
    if (_type == MediaClientType_Pull && rtmpSrc) {
        rtmpSrc->delConnection(this);
        rtmpSrc->release();
    } else if (rtmpSrc) {
        rtmpSrc->delConnection(this);
    }

    if (_playReader) {
        PlayerInfo info;
        info.ip = _socket->getPeerIp();
        info.port = _socket->getPeerPort();
        info.protocol = PROTOCOL_RTMP;
        info.status = "off";
        info.type = _localUrlParser.type_;
        info.uri = _localUrlParser.path_;
        info.vhost = _localUrlParser.vhost_;

        MediaHook::instance()->onPlayer(info);
    }
}

void RtmpClient::init()
{
    MediaClient::registerCreateClient("rtmp", [](MediaClientType type, const std::string &appName, const std::string &streamName){
        return make_shared<RtmpClient>(type, appName, streamName);
    });
}

// void RtmpClient::addRtmpClient(const string& key, const RtmpClient::Ptr& client)
// {
//     logInfo << "add rtmpclient: " << key;
//     lock_guard<mutex> lck(_mapMtx);
//     _mapRtmpClient.emplace(key, client);
// }

// void RtmpClient::delRtmpClient(const string& key)
// {
//     logInfo << "del rtmpclient: " << key;
//     lock_guard<mutex> lck(_mapMtx);
//     _mapRtmpClient.erase(key);
// }

// RtmpClient::Ptr RtmpClient::getRtmpClient(const string& key)
// {
//     lock_guard<mutex> lck(_mapMtx);
//     auto it = _mapRtmpClient.find(key);
//     if (it != _mapRtmpClient.end()) {
//         return it->second;
//     }
    
//     return nullptr;
// }

void RtmpClient::start(const string& localIp, int localPort, const string& url, int timeout)
{
    if (localIp.empty() || url.empty()) {
        return ;
    }
    _url = url;
    _peerUrlParser.parse(url);

    _tcUrl = "rtmp://" + _peerUrlParser.host_;
    if (_peerUrlParser.port_) {
        _tcUrl += ":" + to_string(_peerUrlParser.port_);
    }
    auto pos = _peerUrlParser.path_.find_last_of("/");
    if (pos == string::npos) {
        _tcUrl += _peerUrlParser.path_;
        _peerAppName = _peerUrlParser.path_.substr(1);
    } else {
        _tcUrl += _peerUrlParser.path_.substr(0, pos);
        _peerAppName = _peerUrlParser.path_.substr(1, pos - 1);
        _peerStreamName = _peerUrlParser.path_.substr(1 + pos);
    }

    if (!_peerUrlParser.param_.empty()) {
        _peerStreamName += "?" + _peerUrlParser.param_;
    }

    logInfo << "_peerAppName: " << _peerAppName;
    logInfo << "_peerStreamName: " << _peerStreamName;


    weak_ptr<RtmpClient> wSelf = static_pointer_cast<RtmpClient>(shared_from_this());
    _chunk.setOnRtmpChunk([wSelf](RtmpMessage msg){
        auto self = wSelf.lock();
        if (self) {
            self->onRtmpChunk(msg);
        }
    });
    _handshake = make_shared<RtmpHandshake>(RtmpHandshake::HANDSHAKE_S0S1S2);
    _handshake->setOnHandshake([wSelf](const StreamBuffer::Ptr &buffer){
        auto self = wSelf.lock();
        if (self) {
            self->send(buffer);
        }
    });
    
    _handshake->setOnRtmpChunk([wSelf](const StreamBuffer::Ptr &buffer){
        auto self = wSelf.lock();
        if (self) {
            self->_chunk.parse(buffer);
        }
    });

    logInfo << "localIp: " << localIp << ", localPort: " << localPort;
    if (TcpClient::create(localIp, localPort) < 0) {
        close();
        logInfo << "TcpClient::create failed: " << strerror(errno);
        return ;
    }

    logInfo << "_peerUrlParser.host_: " << _peerUrlParser.host_ << ", _peerUrlParser.port_: " << _peerUrlParser.port_
            << ", timeout: " << timeout;
    _peerUrlParser.port_ = _peerUrlParser.port_ == 0 ? 1935 : _peerUrlParser.port_;
    if (TcpClient::connect(_peerUrlParser.host_, _peerUrlParser.port_, timeout) < 0) {
        close();
        logInfo << "TcpClient::connect, ip: " << _peerUrlParser.host_ << ", peerPort: " 
                << _peerUrlParser.port_ << ", failed: " << strerror(errno);
        return ;
    }
}

void RtmpClient::stop()
{
    close();
}

void RtmpClient::pause()
{
    sendPause();
}

void RtmpClient::onConnect()
{
    _socket = TcpClient::getSocket();
    if (!_socket) {
        return ;
    }
    _loop = _socket->getLoop();
    _chunk.setSocket(_socket);
    sendC0C1();
    _state = RTMP_SEND_C2;
}

void RtmpClient::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    // logInfo << "get buffer size: " << buffer->size();
    if (!_handshake) {
        return ;
    }
    if (_handshake->isCompleted()) {
        // logInfo << "parser chunk";
        _chunk.parse(buffer);
        // logInfo << "parser chunk end";
    } else {
        logInfo << "parser handshake";
        _handshake->parse(buffer);
        if (_handshake->isCompleted()) {
            sendSetChunkSize();
            sendConnect();
        }
    }
}

void RtmpClient::onError(const string& err)
{
    logWarn << "get a err: " << err;
    close();
}

void RtmpClient::close()
{
    if (_onClose) {
        _onClose();
    }

    TcpClient::close();
}

void RtmpClient::sendC0C1()
{
    StreamBuffer::Ptr buffer = make_shared<StreamBuffer>();
    buffer->setCapacity(1 + 1536 + 1);
    _handshake->buildC0C1(buffer->data(), buffer->size());

    send(buffer);
}

void RtmpClient::sendSetChunkSize()
{
    logInfo << "RtmpClient::sendSetChunkSize()";

    _chunk.setOutChunkSize(5000000);
    StreamBuffer::Ptr data = make_shared<StreamBuffer>(5);
    writeUint32BE((char*)data->data(), 5000000);

    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_SET_CHUNK_SIZE;
    rtmp_msg.payload = data;
    rtmp_msg.length = 4;
    sendRtmpChunks(RTMP_CHUNK_CONTROL_ID, rtmp_msg);
}

void RtmpClient::sendConnect()
{
    logInfo << "RtmpClient::sendConnect()";

    AmfObjects objects;    
    _amfEncoder.reset();
    _amfEncoder.encodeString("connect", 7);
    _amfEncoder.encodeNumber(++_sendInvokerId);

    objects["app"] = AmfObject(_peerAppName);
    objects["tcUrl"] = AmfObject(_tcUrl);
    objects["videoFunction"] = AmfObject((double)1);
    objects["audioCodecs"] = AmfObject((double) (0x0400));
    objects["videoCodecs"] = AmfObject((double) (0x0080));
    objects["swfUrl"] = AmfObject(_tcUrl);
    _amfEncoder.encodeObjects(objects);

    sendInvokeMessage(RTMP_CHUNK_INVOKE_ID, _amfEncoder.data(), _amfEncoder.size());

    _state = RTMP_SEND_CREATESTREAM;
}

void RtmpClient::sendCreateStream()
{
    AmfObjects objects;    
    _amfEncoder.reset();
    _amfEncoder.encodeString("createStream", 12);
    _amfEncoder.encodeNumber(++_sendInvokerId);

    _amfEncoder.encodeObjects(objects);

    sendInvokeMessage(RTMP_CHUNK_INVOKE_ID, _amfEncoder.data(), _amfEncoder.size());

    _state = RTMP_SEND_PLAY_PUBLISH;
}

void RtmpClient::sendPlayOrPublish()
{
    AmfObjects objects; 
    if (_type == MediaClientType_Pull) {
        _amfEncoder.reset();
        _amfEncoder.encodeString("play", 4);
        _amfEncoder.encodeNumber(++_sendInvokerId);
        _amfEncoder.encodeObjects(objects);
        _amfEncoder.encodeString(_peerStreamName.data(), _peerStreamName.size());
        _amfEncoder.encodeNumber(_streamId);

        sendInvokeMessage(RTMP_CHUNK_INVOKE_ID, _amfEncoder.data(), _amfEncoder.size());
    } else {
        _amfEncoder.reset();
        _amfEncoder.encodeString("publish", 7);
        _amfEncoder.encodeNumber(++_sendInvokerId);
        _amfEncoder.encodeObjects(objects);
        _amfEncoder.encodeString(_peerStreamName.data(), _peerStreamName.size());
        _amfEncoder.encodeString(_peerAppName.data(), _peerAppName.size());

        sendInvokeMessage(RTMP_CHUNK_INVOKE_ID, _amfEncoder.data(), _amfEncoder.size());
    }
    _state = RTMP_FINISH_PLAY_PUBLISH;
}

void RtmpClient::sendDeleteStream()
{
    
}

void RtmpClient::sendPause()
{

}

void RtmpClient::onRtmpChunk(RtmpMessage msg)
{
    if (!msg.isCompleted()) {
        return ;
    }

    bool ret = true;  
    switch(msg.type_id)
    {        
        case RTMP_VIDEO:
            ret = handleVideo(msg);
            break;
        case RTMP_AUDIO:
            ret = handleAudio(msg);
            break;
        case RTMP_INVOKE:
            logInfo << "handleInvoke";
            ret = handleResponse(msg);
            break;
        case RTMP_NOTIFY:
            logInfo << "handleNotify";
            ret = handleResponse(msg);
            break;
        case RTMP_FLEX_MESSAGE:
            logInfo << "unsupported rtmp flex message";
			ret = false;
            break;            
        case RTMP_SET_CHUNK_SIZE:           
			_chunk.setInChunkSize(readUint32BE(msg.payload->data()));
            break;
		case RTMP_BANDWIDTH_SIZE:
			break;
        case RTMP_FLASH_VIDEO:
            logInfo << "unsupported rtmp flash video";
			ret = false;
            break;    
        case RTMP_ACK:
            break;            
        case RTMP_ACK_SIZE:
            break;
        case RTMP_USER_EVENT:
            // handleUserEvent(msg);
            break;
        default:
            logInfo << "unkonw message type: " << msg.type_id;
            break;
    }

	if (!ret) {
		logInfo << "msg.type_id: " << msg.type_id;
	}
		
    return ;
}

bool RtmpClient::handleResponse(RtmpMessage& rtmp_msg)
{   
    bool ret  = true;
    _amfDecoder.reset();
  
	int bytes_used = _amfDecoder.decode((const char *)rtmp_msg.payload->data(), rtmp_msg.length, 1);
	if (bytes_used < 0) {
		return false;
	}

    std::string method = _amfDecoder.getString();
    
    logInfo << "method: " << method;
    // logInfo << "rtmp_msg.stream_id: " << rtmp_msg.stream_id;
    logInfo << "rtmp_msg.length: " << rtmp_msg.length;
    logInfo << bytes_used << "method: " << bytes_used;
    
    bytes_used += _amfDecoder.decode(rtmp_msg.payload->data()+bytes_used, rtmp_msg.length-bytes_used, 1);
    
    if (method == "_error" || method == "_result") {
        handleCmdResult(bytes_used, rtmp_msg);
    } else if (method == "onStatus") {
        handleCmdOnStatus(bytes_used, rtmp_msg);
    } else if (method == "onMetaData") {
        handleOnMetaData();
    } else {
        logWarn << "method is not support: " << method;
    }

    return ret;
}

void RtmpClient::handleCmdResult(int bytes_used, RtmpMessage& rtmp_msg)
{
    if (_state == RTMP_SEND_CREATESTREAM) {
        // bytes_used += _amfDecoder.decode(rtmp_msg.payload.get()+bytes_used, rtmp_msg.length-bytes_used, 1);
        while (bytes_used < rtmp_msg.length) {
            if (_amfDecoder.hasObject("code")) {
                break;
            }
            bytes_used += _amfDecoder.decode(rtmp_msg.payload->data()+bytes_used, rtmp_msg.length-bytes_used, 1);
        }
        auto code = _amfDecoder.getObject("code").amfString_;
        auto level = _amfDecoder.getObject("level").amfString_;
        if (code != "NetConnection.Connect.Success" || level != "status") {
            logWarn << "result error, code: " << code << ", level: " << level;
            onError("connect failed");
            return ;
        }
        sendCreateStream();
    } else if (_state == RTMP_SEND_PLAY_PUBLISH) {
        // bytes_used += _amfDecoder.decode(rtmp_msg.payload.get()+bytes_used, rtmp_msg.length-bytes_used, 1);
        // bytes_used += _amfDecoder.decode(rtmp_msg.payload.get()+bytes_used, rtmp_msg.length-bytes_used, 1);
        while (bytes_used < rtmp_msg.length) {
            bytes_used += _amfDecoder.decode(rtmp_msg.payload->data()+bytes_used, rtmp_msg.length-bytes_used, 1);
        }

        _streamId = _amfDecoder.getNumber();
        sendPlayOrPublish();
    }
}

void RtmpClient::handleCmdOnStatus(int bytes_used, RtmpMessage& rtmp_msg)
{
    if (_type == MediaClientType_Pull) {
        if (_state == RTMP_FINISH_PLAY_PUBLISH) {
            while (bytes_used < rtmp_msg.length) {
                logInfo << bytes_used << "method: " << bytes_used;
                if (!_amfDecoder.hasObject("code")) {
                    bytes_used += _amfDecoder.decode(rtmp_msg.payload->data()+bytes_used, rtmp_msg.length-bytes_used, 1);
                    continue;
                }
            }

            auto code = _amfDecoder.getObject("code").amfString_;
            auto level = _amfDecoder.getObject("level").amfString_;
            if (/*code != "NetStream.Play.Start" || */level != "status") {
                logWarn << "result error, code: " << code << ", level: " << level;
                onError("play failed");
                return ;
            }

            if (code != "NetStream.Play.Start") {
                logInfo << "code is: " << code;
                return ;
            }

            handlePlay();
        }
    } else if (_type == MediaClientType_Push) {
        if (_state == RTMP_FINISH_PLAY_PUBLISH) {
            while (bytes_used < rtmp_msg.length) {
                logInfo << bytes_used << "method: " << bytes_used;
                if (!_amfDecoder.hasObject("code")) {
                    bytes_used += _amfDecoder.decode(rtmp_msg.payload->data()+bytes_used, rtmp_msg.length-bytes_used, 1);
                    continue;
                }
            }

            auto code = _amfDecoder.getObject("code").amfString_;
            auto level = _amfDecoder.getObject("level").amfString_;
            if (/*code != "NetStream.Publish.Start" || */level != "status") {
                logWarn << "result error, code: " << code << ", level: " << level;
                onError("publish failed");
                return ;
            }

            handlePublish();
        }
    } else {
        logWarn << "client type is not set";
    }
}

bool RtmpClient::handlePlay()
{
    logInfo << "play: " << _localUrlParser.path_;

    auto source = MediaSource::getOrCreate(_localUrlParser.path_, DEFAULT_VHOST
                        , "rtmp", DEFAULT_TYPE
                        , [this](){
                            return make_shared<RtmpMediaSource>(_localUrlParser, _loop);
                        });
    if (!source) {
        onError("get source failed");
        return false;
    }

    auto rtmpSrc = dynamic_pointer_cast<RtmpMediaSource>(source);
    if (!rtmpSrc) {
        return false;
    }
    // rtmpSrc->setSdp(_parser._content);
    rtmpSrc->setOrigin();
    // weak_ptr<RtmpConnection> wSelf = dynamic_pointer_cast<RtmpConnection>(shared_from_this());

    _source = rtmpSrc; 

    lock_guard<mutex> lck(_mtx);
    for (auto &iter : _mapOnReady) {
        rtmpSrc->addOnReady(iter.first, iter.second);
    }
    _mapOnReady.clear();

    return true;
}

void RtmpClient::handleOnMetaData()
{
    _metaData = _amfDecoder.getObjects();
}


bool RtmpClient::handleVideo(RtmpMessage& rtmp_msg)
{
    if (!_validVideoTrack) {
        return false;
    }
    auto rtmpSrc = _source.lock();
    if (!rtmpSrc) {
        close();
        return false;
    }
	uint8_t type = RTMP_VIDEO;
	uint8_t *payload = (uint8_t *)rtmp_msg.payload->data();
	uint32_t length = rtmp_msg.length;
	uint8_t frame_type = (payload[0] >> 4) & 0x0f;
	uint8_t codec_id = payload[0] & 0x0f;

    // shared_ptr<char> tmpPayload(new char[rtmp_msg.length], std::default_delete<char[]>());
    // memcpy(tmpPayload.get(), payload, rtmp_msg.length);
    // rtmp_msg.payload = tmpPayload;
    // payload = (uint8_t *)rtmp_msg.payload.get();

    // logInfo << "frame_type : " << (int)frame_type << ", codec_id: " << (int)codec_id
    //         << ", timestamp: " << rtmp_msg.abs_timestamp;

    // auto server = rtmp_server_.lock();
    // if (!server) {
    //     return false;
    // }

    // auto session = rtmp_session_.lock();
    // if (session == nullptr) {
    //     return false;
    // }

    if (!_rtmpVideoDecodeTrack) {
        _rtmpVideoDecodeTrack = make_shared<RtmpDecodeTrack>(VideoTrackType);
        if (_rtmpVideoDecodeTrack->createTrackInfo(VideoTrackType, codec_id) != 0) {
            _validVideoTrack = false;
            return false;
        }
        rtmpSrc->addTrack(_rtmpVideoDecodeTrack);
    }

    auto msg = make_shared<RtmpMessage>(std::move(rtmp_msg));
    if (!_avcHeader && frame_type == 1/* && codec_id == RTMP_CODEC_ID_H264*/) {
            // logInfo << "payload[1] : " << (int)payload[1];
        if (payload[1] == 0) {
            // sps pps??
            _avcHeaderSize = length;
            _avcHeader = make_shared<StreamBuffer>(length + 1);
            memcpy(_avcHeader->data(), msg->payload->data(), length);
            // session->SetAvcSequenceHeader(avc_sequence_header_, avc_sequence_header_size_);
            rtmpSrc->setAvcHeader(_avcHeader, _avcHeaderSize);
            type = RTMP_AVC_SEQUENCE_HEADER;
            _rtmpVideoDecodeTrack->setConfigFrame(msg);
            rtmpSrc->onReady();
        }
    }
    // } else if (codec_id == RTMP_CODEC_ID_H265) {

    // }

    

    // FILE* fp = fopen("testrtmp.rtmp", "ab+");
    // fwrite(msg->payload.get(), msg->length, 1, fp);
    // fclose(fp);

    msg->trackIndex_ = VideoTrackType;
    _rtmpVideoDecodeTrack->onRtmpPacket(msg);

    // frame_type = (msg->payload.get()[0] >> 4) & 0x0f;
    // codec_id = msg->payload.get()[0] & 0x0f;

    // logInfo << "frame_type : " << (int)frame_type << ", codec_id: " << (int)codec_id
    //         << "msg->payload.get()[0]: " << (int)(msg->payload.get()[0]);

		// session->SendMediaData(type, rtmp_msg._timestamp, rtmp_msg.payload, rtmp_msg.length);

    return true;
}

void RtmpClient::onPublish(const MediaSource::Ptr &src)
{
    if (!src) {
        return ;
    }
    auto rtmpSrc = dynamic_pointer_cast<RtmpMediaSource>(src);
    if (!rtmpSrc) {
        return ;
    }

    _source = rtmpSrc;
    logInfo << "Resetting and playing stream";

    logInfo << "sendMetaData";
    auto metadata = rtmpSrc->getMetadata();
    if (metadata.size() > 0) {
        sendMetaData(metadata);
    }

    if (rtmpSrc->_avcHeader) {
        logInfo << "send a avc header";
        RtmpMessage rtmp_msg;
        rtmp_msg.abs_timestamp = 0;
        rtmp_msg.stream_id = _streamId;
        rtmp_msg.payload = rtmpSrc->_avcHeader;
        rtmp_msg.length = rtmpSrc->_avcHeaderSize;

        rtmp_msg.type_id = RTMP_VIDEO;
        sendRtmpChunks(RTMP_CHUNK_VIDEO_ID, rtmp_msg);
    }

    if (!_playReader) {
        logInfo << "set _playReader";
        weak_ptr<RtmpClient> wSelf = static_pointer_cast<RtmpClient>(shared_from_this());
        _playReader = rtmpSrc->getRing()->attach(_loop, true);
        _playReader->setGetInfoCB([wSelf]() {
            auto self = wSelf.lock();
            ClientInfo ret;
            if (!self) {
                return ret;
            }
            ret.ip_ = self->_socket->getLocalIp();
            ret.port_ = self->_socket->getLocalPort();
            ret.protocol_ = PROTOCOL_RTMP;
            return ret;
        });
        _playReader->setDetachCB([wSelf]() {
            auto strong_self = wSelf.lock();
            if (!strong_self) {
                return;
            }
            // strong_self->shutdown(SockException(Err_shutdown, "rtsp ring buffer detached"));
            strong_self->close();
        });
        logInfo << "setReadCB =================";
        _playReader->setReadCB([wSelf](const RtmpMediaSource::RingDataType &pack) {
            auto self = wSelf.lock();
            if (!self/* || pack->empty()*/) {
                return;
            }
            logInfo << "send rtmp msg";
            auto pktList = *(pack.get());
            for (auto& pkt : pktList) {
                uint8_t frame_type = (pkt->payload->data()[0] >> 4) & 0x0f;
				uint8_t codec_id = pkt->payload->data()[0] & 0x0f;

                // logInfo << "send rtmp msg,time: " << pkt->abs_timestamp << ", type: " << (int)(pkt->type_id)
                //             << ", length: " << pkt->length;
                self->sendRtmpChunks(pkt->csid, *pkt);
            }
        });

        PlayerInfo info;
        info.ip = _socket->getPeerIp();
        info.port = _socket->getPeerPort();
        info.protocol = PROTOCOL_RTMP;
        info.status = "on";
        info.type = _localUrlParser.type_;
        info.uri = _localUrlParser.path_;
        info.vhost = _localUrlParser.vhost_;

        MediaHook::instance()->onPlayer(info);
    }
}

bool RtmpClient::sendMetaData(AmfObjects meta_data)
{
    // if(this->isClosed()) {
    //     return false;
    // }

	if (meta_data.size() == 0) {
		return false;
	}

    _amfEncoder.reset(); 
    _amfEncoder.encodeString("onMetaData", 10);
    _amfEncoder.encodeECMA(meta_data);

    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_NOTIFY;
    rtmp_msg.timestamp = 0;
    rtmp_msg.stream_id = RTMP_StreamID_CONTROL;
    rtmp_msg.payload = _amfEncoder.data();
    rtmp_msg.length = _amfEncoder.size(); 
    sendRtmpChunks(RTMP_CHUNK_DATA_ID, rtmp_msg);

    return true;
}

bool RtmpClient::handlePublish()
{
    weak_ptr<RtmpClient> wSelf = static_pointer_cast<RtmpClient>(shared_from_this());

    MediaSource::getOrCreateAsync(_localUrlParser.path_, _localUrlParser.vhost_, _localUrlParser.protocol_, _localUrlParser.type_, 
    [wSelf](const MediaSource::Ptr &src){
        logInfo << "get a src";
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        if (!src) {
            self->onError("source is empty");
            return ;
        }

        self->_loop->async([wSelf, src](){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            self->onPublish(src);
        }, true);
    }, 
    [wSelf]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }
        return make_shared<RtmpMediaSource>(self->_localUrlParser, nullptr, true);
    }, this);

    return true;
}

bool RtmpClient::handleAudio(RtmpMessage& rtmp_msg)
{
    if (!_validAudioTrack) {
        return false;
    }
    auto rtmpSrc = _source.lock();
    if (!rtmpSrc) {
        close();
        return false;
    }
	// uint8_t type = RTMP_AUDIO;
	uint8_t *payload = (uint8_t *)rtmp_msg.payload->data();
	uint32_t length = rtmp_msg.length;
	uint8_t sound_format = (payload[0] >> 4) & 0x0f;
	//uint8_t sound_size = (payload[0] >> 1) & 0x01;
	//uint8_t sound_rate = (payload[0] >> 2) & 0x03;
	uint8_t codec_id = payload[0] & 0x0f;

    // auto server = rtmp_server_.lock();
    // if (!server) {
    //     return false;
    // }

    // auto session = rtmp_session_.lock();
    // if (session == nullptr) {
    //     return false;
    // }

    if (!_rtmpAudioDecodeTrack) {
        _rtmpAudioDecodeTrack = make_shared<RtmpDecodeTrack>(AudioTrackType);
        if (_rtmpAudioDecodeTrack->createTrackInfo(AudioTrackType, sound_format) != 0) {
            _validAudioTrack = false;
            return false;
        }
        rtmpSrc->addTrack(_rtmpAudioDecodeTrack);
    }

    auto msg = make_shared<RtmpMessage>(std::move(rtmp_msg));
    if (!_aacHeader && sound_format == RTMP_CODEC_ID_AAC && payload[1] == 0) {
        _aacHeaderSize = msg->length;
        _aacHeader = make_shared<StreamBuffer>(length + 1);
        memcpy(_aacHeader->data(), msg->payload->data(), msg->length);
        // session->SetAacSequenceHeader(aac_sequence_header_, aac_sequence_header_size_);
        // type = RTMP_AAC_SEQUENCE_HEADER;
        rtmpSrc->setAacHeader(_aacHeader, _aacHeaderSize);
        _rtmpAudioDecodeTrack->setConfigFrame(msg);
        rtmpSrc->onReady();
    } else if (sound_format != RTMP_CODEC_ID_AAC) {
        rtmpSrc->onReady();
    }

    msg->trackIndex_ = AudioTrackType;
    _rtmpAudioDecodeTrack->onRtmpPacket(msg);

    // session->SendMediaData(type, rtmp_msg._timestamp, rtmp_msg.payload, rtmp_msg.length);

    return true;
}

bool RtmpClient::sendInvokeMessage(uint32_t csid, const StreamBuffer::Ptr& payload, uint32_t payload_size)
{
    // if(this->IsClosed()) {
    //     return false;
    // }

    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_INVOKE;
    rtmp_msg.timestamp = 0;
    rtmp_msg.stream_id = _streamId;
    rtmp_msg.payload = payload;
    rtmp_msg.length = payload_size; 
    sendRtmpChunks(csid, rtmp_msg);  
    return true;
}

void RtmpClient::sendRtmpChunks(uint32_t csid, RtmpMessage& rtmp_msg)
{    
    _chunk.createChunk(csid, rtmp_msg);
}

void RtmpClient::setOnClose(const function<void()>& cb)
{
    _onClose = cb;
}

void RtmpClient::addOnReady(void* key, const function<void()>& onReady)
{
    lock_guard<mutex> lck(_mtx);
    auto rtmpSrc =_source.lock();
    if (rtmpSrc) {
        rtmpSrc->addOnReady(key, onReady);
        return ;
    }
    _mapOnReady.emplace(key, onReady);
}