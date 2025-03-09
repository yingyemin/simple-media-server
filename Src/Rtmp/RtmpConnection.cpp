#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "RtmpConnection.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"
#include "Rtmp.h"
#include "Common/Config.h"
#include "Hook/MediaHook.h"
#include "Codec/AacFrame.h"
#include "Codec/AacTrack.h"

#include <sstream>

using namespace std;

RtmpConnection::RtmpConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket)
    :TcpConnection(loop, socket)
    ,_loop(loop)
    ,_socket(socket)
{
    logInfo << "RtmpConnection: " << this;
}

RtmpConnection::~RtmpConnection()
{
    logInfo << "~RtmpConnection: " << this << ", path: " << _urlParser.path_;
    auto rtmpSrc = _source.lock();
    if (_isPublish && rtmpSrc) {
        rtmpSrc->delConnection(this);
        rtmpSrc->release();
        rtmpSrc->delOnDetach(this);
    } else if (rtmpSrc) {
        rtmpSrc->delConnection(this);
        rtmpSrc->delOnDetach(this);
    }

    if (_playReader) {
        PlayerInfo info;
        info.ip = _socket->getPeerIp();
        info.port = _socket->getPeerPort();
        info.protocol = PROTOCOL_RTMP;
        info.status = "off";
        info.type = _urlParser.type_;
        info.uri = _urlParser.path_;
        info.vhost = _urlParser.vhost_;

        MediaHook::instance()->onPlayer(info);
    }
}

void RtmpConnection::init()
{
    weak_ptr<RtmpConnection> wSelf = static_pointer_cast<RtmpConnection>(shared_from_this());
    _chunk.setOnRtmpChunk([wSelf](RtmpMessage msg){
        auto self = wSelf.lock();
        if (self) {
            self->onRtmpChunk(msg);
        }
    });
    _chunk.setSocket(_socket);
    _handshake = make_shared<RtmpHandshake>(RtmpHandshake::HANDSHAKE_C0C1);
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
}

void RtmpConnection::close()
{
    TcpConnection::close();
}

void RtmpConnection::onManager()
{
    logTrace << "manager, path: " << _urlParser.path_;
}

void RtmpConnection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    // logInfo << "get a buf: " << buffer->size();
    // _parser.parse(buffer->data(), buffer->size());
    if (_handshake->isCompleted()) {
        // logInfo << "parser chunk";
        _chunk.parse(buffer);
        // logInfo << "parser chunk end";
    } else {
        logTrace << "parser handshake";
        _handshake->parse(buffer);
    }
}

void RtmpConnection::onError()
{
    close();
    logWarn << "get a error: ";
}

ssize_t RtmpConnection::send(Buffer::Ptr pkt)
{
    // logInfo << "pkt size: " << pkt->size();
    return TcpConnection::send(pkt);
}

void RtmpConnection::onRtmpChunk(RtmpMessage msg)
{
    if (!msg.isCompleted()) {
        return ;
    }

    bool ret = true;  
    logTrace << "get rtmp msg: " << (int)msg.type_id;
    switch(msg.type_id)
    {        
        case RTMP_VIDEO:
            ret = handleVideo(msg);
            break;
        case RTMP_AUDIO:
            ret = handleAudio(msg);
            break;
        case RTMP_INVOKE:
            logTrace << "handleInvoke";
            ret = handleInvoke(msg);
            break;
        case RTMP_NOTIFY:
            logTrace << "handleNotify";
            ret = handleNotify(msg);
            break;
        case RTMP_FLEX_MESSAGE:
            logTrace << "unsupported rtmp flex message";
			ret = false;
            break;            
        case RTMP_SET_CHUNK_SIZE:           
			_chunk.setInChunkSize(readUint32BE(msg.payload->data()));
            break;
		case RTMP_BANDWIDTH_SIZE:
			break;
        case RTMP_FLASH_VIDEO:
            logTrace << "unsupported rtmp flash video";
			ret = false;
            break;    
        case RTMP_ACK:
            break;            
        case RTMP_ACK_SIZE:
            break;
        case RTMP_USER_EVENT:
            handleUserEvent(msg);
            break;
        default:
            logInfo << "unkonw message type: " << (int)msg.type_id;
            break;
    }

	if (!ret) {
		logInfo << "msg.type_id: " << (int)msg.type_id;
	}
		
    return ;
}

bool RtmpConnection::handleInvoke(RtmpMessage& rtmp_msg)
{   
    bool ret  = true;
    _amfDecoder.reset();
  
	int bytes_used = _amfDecoder.decode((const char *)rtmp_msg.payload->data(), rtmp_msg.length, 1);
	if (bytes_used < 0) {
		return false;
	}

    std::string method = _amfDecoder.getString();
	//LOG_INFO("[Method] %s\n", method.c_str());
    logDebug << "method: " << method;
    logTrace << "rtmp_msg.stream_id: " << rtmp_msg.stream_id;
    logTrace << "_streamId: " << _streamId;
    if(rtmp_msg.stream_id == 0) {
        bytes_used += _amfDecoder.decode(rtmp_msg.payload->data()+bytes_used, rtmp_msg.length-bytes_used);
        if(method == "connect") {            
            ret = handleConnect();
        }
        else if(method == "createStream") {      
            ret = handleCreateStream();
        }
    }
    else if(rtmp_msg.stream_id == _streamId) {
        bytes_used += _amfDecoder.decode((const char *)rtmp_msg.payload->data()+bytes_used, rtmp_msg.length-bytes_used, 3);
        _streamName = _amfDecoder.getString();
        // _path = "/" + _app + "/" + _streamName;
        _tcUrl += "/" + _streamName;
        // logInfo << "get a path: " << _path;
        logInfo << "get a _tcUrl: " << _tcUrl;

        _urlParser.parse(_tcUrl);
        // logInfo << "_urlParser.path_: " << _urlParser.path_;
        // logInfo << "_urlParser.type: " << _urlParser.type_;
        _path = _urlParser.path_;
    
        if((int)rtmp_msg.length > bytes_used) {
            bytes_used += _amfDecoder.decode((const char *)rtmp_msg.payload->data()+bytes_used, rtmp_msg.length-bytes_used);                      
        }
            
        if(method == "publish" || method == "FCPublish") {
            PublishInfo info;
            info.protocol = _urlParser.protocol_;
            info.type = _urlParser.type_;
            info.uri = _urlParser.path_;
            info.vhost = _urlParser.vhost_;
            info.params = _urlParser.param_;

            weak_ptr<RtmpConnection> wSelf = dynamic_pointer_cast<RtmpConnection>(shared_from_this());
            MediaHook::instance()->onPublish(info, [wSelf](const PublishResponse &rsp){
                auto self = wSelf.lock();
                if (!self) {
                    return ;
                }

                if (!rsp.authResult) {
                    self->onError();
                    return ;
                }

                self->handlePublish();
            });
            ret = true;
        }
        else if(method == "play") {   
            PlayInfo info;
            info.protocol = _urlParser.protocol_;
            info.type = _urlParser.type_;
            info.uri = _urlParser.path_;
            info.vhost = _urlParser.vhost_;

            weak_ptr<RtmpConnection> wSelf = dynamic_pointer_cast<RtmpConnection>(shared_from_this());
            MediaHook::instance()->onPlay(info, [wSelf](const PlayResponse &rsp){
                auto self = wSelf.lock();
                if (!self) {
                    return ;
                }

                if (!rsp.authResult) {
                    self->onError();
                    return ;
                }

                self->handlePlay();
            });       
            ret = true;
        }
        else if(method == "play2") {         
            ret = handlePlay2();
        }
        else if(method == "DeleteStream") {
            ret = handleDeleteStream();
        } else if (method == "releaseStream") {

        } else if (method == "seek") {

        } else if (method == "pause") {
            
        }
    }

    return ret;
}

bool RtmpConnection::handleNotify(RtmpMessage& rtmp_msg)
{   
    _amfDecoder.reset();
    int bytes_used = _amfDecoder.decode((const char *)rtmp_msg.payload->data(), rtmp_msg.length, 1);
    if(bytes_used < 0) {
        return false;
    }

    logTrace << "_amfDecoder.getString(): " << _amfDecoder.getString();
    if(_amfDecoder.getString() == "@setDataFrame")
    {
        _amfDecoder.reset();
        bytes_used = _amfDecoder.decode((const char *)rtmp_msg.payload->data()+bytes_used, rtmp_msg.length-bytes_used, 1);
        if(bytes_used < 0) {           
            return false;
        }

        logTrace << "_amfDecoder.getString(): " << _amfDecoder.getString();
        if(_amfDecoder.getString() == "onMetaData") {
            _amfDecoder.decode((const char *)rtmp_msg.payload->data()+bytes_used, rtmp_msg.length-bytes_used);
            _metaData = _amfDecoder.getObjects();

            int audiosamplerate = 0;
            int audiochannels = 0;
            int audiosamplesize = 0;
            int videodatarate = 0;
            int audiodatarate = 0;
            int duration = 0;
            int videocodecid = 0;
            int audiocodecid = 0;
            for (auto& meta : _metaData) {
                string key = meta.first;
                auto val = meta.second;

                if (key == "duration") {
                    duration = (float)val.amfNumber_;
                } else if (key == "audiosamplerate") {
                    audiosamplerate = val.amfNumber_;
                } else if (key == "audiosamplesize") {
                    audiosamplesize = val.amfNumber_;
                } else if (key == "stereo") {
                    audiochannels = val.amfBoolean_ ? 2 : 1;
                } else if (key == "videocodecid") {
                    // 找到视频  [AUTO-TRANSLATED:e66249fc]
                    // Find video
                    if ((int)val.amfNumber_ != 0) {
                        videocodecid = val.amfNumber_;
                    } else if (val.type_ = AMF_STRING) {
                        videocodecid = getIdByCodecName(VideoTrackType, val.amfString_);
                    }
                } else if (key == "audiocodecid") {
                    // 找到音频  [AUTO-TRANSLATED:126ce656]
                    // Find audio
                    if ((int)val.amfNumber_ != 0) {
                        audiocodecid = val.amfNumber_;
                    } else if (val.type_ = AMF_STRING) {
                        audiocodecid = getIdByCodecName(AudioTrackType, val.amfString_);
                    }
                } else if (key == "audiodatarate") {
                    audiodatarate = val.amfNumber_;
                } else if (key == "videodatarate") {
                    videodatarate = val.amfNumber_;
                }
            }

            auto rtmpSrc = _source.lock();
            if (!rtmpSrc) {
                close();
                return false;
            }

            if (!_rtmpAudioDecodeTrack && audiocodecid) {
                _rtmpAudioDecodeTrack = make_shared<RtmpDecodeTrack>(AudioTrackType);
                if (_rtmpAudioDecodeTrack->createTrackInfo(AudioTrackType, audiocodecid) != 0) {
                    _validAudioTrack = false;
                    return false;
                }
                auto trackInfo = _rtmpAudioDecodeTrack->getTrackInfo();
                trackInfo->samplerate_ = audiosamplerate;
                trackInfo->bitPerSample_ = audiosamplesize;
                trackInfo->channel_ = audiochannels;
                rtmpSrc->addTrack(_rtmpAudioDecodeTrack);
            }

            if (!_rtmpVideoDecodeTrack && videocodecid) {
                _rtmpVideoDecodeTrack = make_shared<RtmpDecodeTrack>(VideoTrackType);
                if (videocodecid == fourccH265) {
                    videocodecid = RTMP_CODEC_ID_H265;
                } else if (videocodecid == fourccAV1) {
                    videocodecid = RTMP_CODEC_ID_AV1;
                } else if (videocodecid == fourccVP9) {
                    videocodecid = RTMP_CODEC_ID_VP9;
                }
                if (_rtmpVideoDecodeTrack->createTrackInfo(VideoTrackType, videocodecid) != 0) {
                    // _validVideoTrack = false;
                    return false;
                }
                rtmpSrc->addTrack(_rtmpVideoDecodeTrack);
            }
        }
    }

    return true;
}

bool RtmpConnection::handleUserEvent(RtmpMessage& rtmp_msg)
{
    char *payload = rtmp_msg.payload->data();
    uint32_t length = rtmp_msg.length;

    uint16_t event_type = readUint16BE(payload);
    payload += 2;
    length -= 2;

    switch (event_type) {
        case 6: //CONTROL_PING_REQUEST
        {
            if (length < 4) {
                return false;
            }
            uint32_t timeStamp = readUint32BE(payload);
            break;
        }
        case 7: //CONTROL_PING_RESPONSE
        {
            if (length < 4) {
                return false;
            }
            uint32_t timeStamp = readUint32BE(payload);
            break;
        }
        case 0: //play
        {
            if (length < 4) {
                return false;
            }
            uint32_t index = readUint32BE(payload);
            break;
        }
        case 1: //pause
        {
            if (length < 4) {
                return false;
            }
            uint32_t index = readUint32BE(payload);
            break;
        }
        case 2: //stop
        {
            if (length < 4) {
                return false;
            }
            uint32_t index = readUint32BE(payload);
            break;
        }
        default:
            break;
    }

    return true;
}

bool RtmpConnection::handleVideo(RtmpMessage& rtmp_msg)
{
    // static uint32_t fourccH265 = (unsigned)('h') << 24 | 'v' << 16 | 'c' << 8 | '1';
    // static uint32_t fourccAV1 = (unsigned)('a') << 24 | 'v' << 16 | '0' << 8 | '1';
    // static uint32_t fourccVP9 = (unsigned)('v') << 24 | 'p' << 16 | '0' << 8 | '9';

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
    if (length < 2) {
        return false;
    }
    
    bool isEnhance = (payload[0] >> 4) & 0b1000;
	uint8_t frame_type;
	uint8_t codec_id;
    uint8_t packet_type;

    if (isEnhance) {
        if (length < 5) {
            return false;
        }
        frame_type = (payload[0] >> 4) & 0b0111;
        packet_type = payload[0] & 0x0f;
        if (readUint32BE((char*)payload + 1) == fourccH265) {
            codec_id = RTMP_CODEC_ID_H265;
        } else if (readUint32BE((char*)payload + 1) == fourccAV1) {
            codec_id = RTMP_CODEC_ID_AV1;
        } else if (readUint32BE((char*)payload + 1) == fourccVP9) {
            codec_id = RTMP_CODEC_ID_VP9;
        } else {
            close();
            return false;
        }
    } else {
        frame_type = (payload[0] >> 4) & 0x0f;
        codec_id = payload[0] & 0x0f;
        packet_type = payload[1];
    }

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
        if (packet_type == 0) {
            // sps pps??
            _avcHeaderSize = length;
            _avcHeader = make_shared<StreamBuffer>(length + 1);;
            memcpy(_avcHeader->data(), msg->payload->data(), length);
            // session->SetAvcSequenceHeader(avc_sequence_header_, avc_sequence_header_size_);
            rtmpSrc->setAvcHeader(_avcHeader, _avcHeaderSize);
            // type = RTMP_AVC_SEQUENCE_HEADER;
            _rtmpVideoDecodeTrack->setConfigFrame(msg);
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

bool RtmpConnection::handleAudio(RtmpMessage& rtmp_msg)
{
    if (!_validAudioTrack) {
        return false;
    }
    auto rtmpSrc = _source.lock();
    if (!rtmpSrc) {
        close();
        return false;
    }
	uint8_t type = RTMP_AUDIO;
	uint8_t *payload = (uint8_t *)rtmp_msg.payload->data();
	uint32_t length = rtmp_msg.length;
    if (length < 2) {
        return false;
    }
	uint8_t sound_format = (payload[0] >> 4) & 0x0f;
    // 0:单声道， 1：双声道
    uint8_t sound_channel  = payload[0] & 0x01;
    // 0：8bit， 1：16bit
	uint8_t sound_size = (payload[0] >> 1) & 0x01;
    // 0：5.5kHz， 1：11kHz， 2：22kHz， 3：44kHz
	uint8_t sound_rate = (payload[0] >> 2) & 0x03;
	// uint8_t codec_id = payload[0] & 0x0f;

    static int channelArray[] = {1, 2};
    static int bitArray[] = {8, 16};
    static int sampleArray[] = {5512, 11025, 22050, 44100};

    // logInfo << "sound_channel: " << (int)sound_channel
    //         << ", sound_size: " << (int)sound_size
    //         << ", sound_rate: " << (int)sound_rate;

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
        if (sound_format == RTMP_CODEC_ID_AAC || sound_format == RTMP_CODEC_ID_MP3
            || sound_format == RTMP_CODEC_ID_OPUS) {
            auto trackInfo = _rtmpAudioDecodeTrack->getTrackInfo();
            trackInfo->samplerate_ = sampleArray[sound_rate];
            trackInfo->bitPerSample_ = bitArray[sound_size];
            trackInfo->channel_ = channelArray[sound_channel];
        }
        rtmpSrc->addTrack(_rtmpAudioDecodeTrack);
    }

    auto msg = make_shared<RtmpMessage>(std::move(rtmp_msg));
    if (!_aacHeader && sound_format == RTMP_CODEC_ID_AAC && payload[1] == 0) {
        _aacHeaderSize = msg->length;
        _aacHeader = make_shared<StreamBuffer>(length + 1);
        memcpy(_aacHeader->data(), msg->payload->data(), msg->length);
        // session->SetAacSequenceHeader(aac_sequence_header_, aac_sequence_header_size_);
        type = RTMP_AAC_SEQUENCE_HEADER;
        rtmpSrc->setAacHeader(_aacHeader, _aacHeaderSize);
        _rtmpAudioDecodeTrack->setConfigFrame(msg);

        return true;
    }

    msg->trackIndex_ = AudioTrackType;
    _rtmpAudioDecodeTrack->onRtmpPacket(msg);

    // session->SendMediaData(type, rtmp_msg._timestamp, rtmp_msg.payload, rtmp_msg.length);

    return true;
}

bool RtmpConnection::handleConnect()
{
    if(!_amfDecoder.hasObject("app")) {
        return false;
    }

    AmfObject amfObj = _amfDecoder.getObject("app");
    _app = amfObj.amfString_;
    if(_app == "") {
        return false;
    }

    if(!_amfDecoder.hasObject("tcUrl")) {
        return false;
    }

    amfObj = _amfDecoder.getObject("tcUrl");
    _tcUrl = amfObj.amfString_;
    if(_tcUrl == "") {
        _tcUrl = string(PROTOCOL_RTMP) + "://" + DEFAULT_VHOST + "/" + _app;
    } else {
        UrlParser tcUrlParser;
        tcUrlParser.parse(_tcUrl);
        _tcUrl = string(PROTOCOL_RTMP) + "://" + tcUrlParser.host_ + "/" + _app;
    }

    setChunkSize();
    sendAcknowledgement();
    setPeerBandwidth();   

    AmfObjects objects;    
    _amfEncoder.reset();
    _amfEncoder.encodeString("_result", 7);
    _amfEncoder.encodeNumber(_amfDecoder.getNumber());

    objects["fmsVer"] = AmfObject(std::string("FMS/4,5,0,297"));
    objects["capabilities"] = AmfObject(255.0);
    objects["mode"] = AmfObject(1.0);
    _amfEncoder.encodeObjects(objects);
    objects.clear();
    objects["level"] = AmfObject(std::string("status"));
    objects["code"] = AmfObject(std::string("NetConnection.Connect.Success"));
    objects["description"] = AmfObject(std::string("Connection succeeded."));
    objects["objectEncoding"] = AmfObject(0.0);
    _amfEncoder.encodeObjects(objects);  

    sendInvokeMessage(RTMP_CHUNK_INVOKE_ID, _amfEncoder.data(), _amfEncoder.size());
    return true;
}

bool RtmpConnection::handleCreateStream()
{ 
	int streamId = _chunk.getStreamId();

	AmfObjects objects;
	_amfEncoder.reset();
	_amfEncoder.encodeString("_result", 7);
	_amfEncoder.encodeNumber(_amfDecoder.getNumber());
	_amfEncoder.encodeObjects(objects);
	_amfEncoder.encodeNumber(streamId);

	sendInvokeMessage(RTMP_CHUNK_INVOKE_ID, _amfEncoder.data(), _amfEncoder.size());
	_streamId = streamId;
	return true;
}

bool RtmpConnection::handlePublish()
{
    logInfo << "publish: " << _path;
    //LOG_INFO("[Publish] app: %s, stream name: %s, stream path: %s\n", app_.c_str(), stream_name_.c_str(), stream_path_.c_str());

	// auto server = rtmp_server_.lock();
	// if (!server) {
	// 	return false;
	// }

    AmfObjects objects; 
    _amfEncoder.reset();
    _amfEncoder.encodeString("onStatus", 8);
    _amfEncoder.encodeNumber(0);
    _amfEncoder.encodeObjects(objects);

    bool is_error = false;

    do {
        if (_source.lock() || _isPublish) {
            is_error = true;
            objects["level"] = AmfObject(std::string("error"));
            objects["code"] = AmfObject(std::string("NetStream.Publish.BadName"));
            objects["description"] = AmfObject(std::string("Stream already publishing."));
            break;
        }

        // _urlParser.path_ = _path;
        // _urlParser.type_ = DEFAULT_TYPE;
        // _urlParser.protocol_ = "rtmp";
        // _urlParser.vhost_ = DEFAULT_VHOST;
        auto source = MediaSource::getOrCreate(_urlParser.path_, _urlParser.vhost_
                            , _urlParser.protocol_, _urlParser.type_
                            , [this](){
                                return make_shared<RtmpMediaSource>(_urlParser, _loop);
                            });
        if (!source) {
            is_error = true;
            objects["level"] = AmfObject(std::string("error"));
            objects["code"] = AmfObject(std::string("NetStream.Publish.BadName"));
            objects["description"] = AmfObject(std::string("Stream already publishing."));
            break;
        }

        auto rtmpSrc = dynamic_pointer_cast<RtmpMediaSource>(source);
        if (!rtmpSrc) {
            is_error = true;
            objects["level"] = AmfObject(std::string("error"));
            objects["code"] = AmfObject(std::string("NetStream.Publish.BadName"));
            objects["description"] = AmfObject(std::string("Get Source Failed."));
            break;
        }
        // rtmpSrc->setSdp(_parser._content);
        rtmpSrc->setOrigin();
        rtmpSrc->setOriginSocket(_socket);
        rtmpSrc->setAction(true);
        weak_ptr<RtmpConnection> wSelf = dynamic_pointer_cast<RtmpConnection>(shared_from_this());
        rtmpSrc->addOnDetach(this, [wSelf](){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }
            self->close();
        });

        _source = rtmpSrc;
        // _rtmpDecodeTrack = make_shared<RtmpDecodeTrack>(0);
        // rtmpSrc->addTrack(_rtmpDecodeTrack);
        // rtmpSrc->setStatus(AVAILABLE);

        /*if(server->HasPublisher(stream_path_)) {
            is_error = true;
            objects["level"] = AmfObject(std::string("error"));
            objects["code"] = AmfObject(std::string("NetStream.Publish.BadName"));
            objects["description"] = AmfObject(std::string("Stream already publishing."));
        }
        // else */
        // if(_isPublish) {
        //     is_error = true;
        //     objects["level"] = AmfObject(std::string("error"));
        //     objects["code"] = AmfObject(std::string("NetStream.Publish.BadConnection"));
        //     objects["description"] = AmfObject(std::string("Connection already publishing."));
        // }
        // /* else if(0)  {
        //     // 认证处理 
        // } */
        // else {
            objects["level"] = AmfObject(std::string("status"));
            objects["code"] = AmfObject(std::string("NetStream.Publish.Start"));
            objects["description"] = AmfObject(std::string("Start publising."));
            // server->AddSession(stream_path_);
            // rtmp_session_ = server->GetSession(stream_path_);

            // if (server) {
            // 	server->NotifyEvent("publish.start", stream_path_);
            // }
        // }
    } while (0);

    _amfEncoder.encodeObjects(objects);     
    sendInvokeMessage(RTMP_CHUNK_INVOKE_ID, _amfEncoder.data(), _amfEncoder.size());

    if(is_error) {
        close();
    }
    else {
        // connection_state_ = START_PUBLISH;
		_isPublish = true;
    }

    // auto session = rtmp_session_.lock();
    // if(session) {
	// 	session->SetGopCache(max_gop_cache_len_);
	// 	session->AddSink(std::dynamic_pointer_cast<RtmpSink>(shared_from_this()));
    // }        

    return true;
}

void RtmpConnection::responsePlay(const MediaSource::Ptr &src)
{
    AmfObjects objects; 
    if (!src || !dynamic_pointer_cast<RtmpMediaSource>(src)) {
        logInfo << "No such stream";
        _amfEncoder.reset(); 
        _amfEncoder.encodeString("onStatus", 8);
        _amfEncoder.encodeNumber(0);
        objects["level"] = AmfObject(std::string("error"));
        objects["code"] = AmfObject(std::string("NetStream.Play.StreamNotFound"));
        objects["description"] = AmfObject(std::string("No such stream."));
        _amfEncoder.encodeObjects(objects);
        sendInvokeMessage(RTMP_CHUNK_INVOKE_ID, _amfEncoder.data(), _amfEncoder.size());
        close();
        return;
    }

    auto rtmpSrc = dynamic_pointer_cast<RtmpMediaSource>(src);
    if (!rtmpSrc) {
        logInfo << "No such stream";
        _amfEncoder.reset(); 
        _amfEncoder.encodeString("onStatus", 8);
        _amfEncoder.encodeNumber(0);
        objects["level"] = AmfObject(std::string("error"));
        objects["code"] = AmfObject(std::string("NetStream.Play.StreamNotFound"));
        objects["description"] = AmfObject(std::string("No such stream."));
        _amfEncoder.encodeObjects(objects);
        sendInvokeMessage(RTMP_CHUNK_INVOKE_ID, _amfEncoder.data(), _amfEncoder.size());
        close();
        return;
    }

    _source = rtmpSrc;
    logInfo << "Resetting and playing stream";

    _amfEncoder.reset(); 
    _amfEncoder.encodeString("onStatus", 8);
    _amfEncoder.encodeNumber(0);
    _amfEncoder.encodeObjects(objects);
    objects["level"] = AmfObject(std::string("status"));
    objects["code"] = AmfObject(std::string("NetStream.Play.Reset"));
    objects["description"] = AmfObject(std::string("Resetting and playing stream."));
    _amfEncoder.encodeObjects(objects);   
    if(!sendInvokeMessage(RTMP_CHUNK_INVOKE_ID, _amfEncoder.data(), _amfEncoder.size())) {
        return ;
    }

    logInfo << "Started playing";
    objects.clear(); 
    _amfEncoder.reset(); 
    _amfEncoder.encodeString("onStatus", 8);
    _amfEncoder.encodeNumber(0);    
    _amfEncoder.encodeObjects(objects);
    objects["level"] = AmfObject(std::string("status"));
    objects["code"] = AmfObject(std::string("NetStream.Play.Start"));
    objects["description"] = AmfObject(std::string("Started playing."));   
    _amfEncoder.encodeObjects(objects);
    if(!sendInvokeMessage(RTMP_CHUNK_INVOKE_ID, _amfEncoder.data(), _amfEncoder.size())) {
        return ;
    }

    logInfo << "RtmpSampleAccess";
    _amfEncoder.reset(); 
    _amfEncoder.encodeString("|RtmpSampleAccess", 17);
    _amfEncoder.encodeBoolean(true);
    _amfEncoder.encodeBoolean(true);
    if(!sendNotifyMessage(RTMP_CHUNK_DATA_ID, _amfEncoder.data(), _amfEncoder.size())) {
        return ;
    }

    _isPlay = true;
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

    if (rtmpSrc->_aacHeader) {
        logInfo << "send a aac header";
        RtmpMessage rtmp_msg;
        rtmp_msg.abs_timestamp = 0;
        rtmp_msg.stream_id = _streamId;
        rtmp_msg.payload = rtmpSrc->_aacHeader;
        rtmp_msg.length = rtmpSrc->_aacHeaderSize;

        rtmp_msg.type_id = RTMP_AUDIO;
        sendRtmpChunks(RTMP_CHUNK_AUDIO_ID, rtmp_msg);
    }

    static bool enbaleAddMute = Config::instance()->getAndListen([](const json &config){
        enbaleAddMute = Config::instance()->get("Rtmp", "Server", "Server1", "enableAddMute");
    }, "Rtmp", "Server", "Server1", "enableAddMute");
    auto tracks = rtmpSrc->getTrackInfo();
    if (enbaleAddMute && tracks.size() == 1 && tracks.begin()->second->trackType_ == "video") {
        _addMute = true;
        logInfo << "send a aac header";
        RtmpMessage rtmp_msg;
        rtmp_msg.abs_timestamp = 0;
        rtmp_msg.stream_id = _streamId;
        rtmp_msg.payload = AacTrack::getMuteConfig();
        rtmp_msg.length = rtmp_msg.payload->size();

        rtmp_msg.type_id = RTMP_AUDIO;
        sendRtmpChunks(RTMP_CHUNK_AUDIO_ID, rtmp_msg);
    }

    if (!_playReader) {
        logInfo << "set _playReader";
        static int interval = Config::instance()->getAndListen([](const json &config){
            interval = Config::instance()->get("Rtmp", "Server", "Server1", "interval");
            if (interval == 0) {
                interval = 5000;
            }
        }, "Rtmp", "Server", "Server1", "interval");

        if (interval == 0) {
            interval = 5000;
        }
        weak_ptr<RtmpConnection> wSelf = static_pointer_cast<RtmpConnection>(shared_from_this());
        _loop->addTimerTask(interval, [wSelf](){
            auto self = wSelf.lock();
            if (!self) {
                return 0;
            }
            self->_lastBitrate = self->_intervalSendBytes / (interval / 1000.0);
            self->_intervalSendBytes = 0;

            return interval;
        }, nullptr);

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
            ret.bitrate_ = self->_lastBitrate;
            ret.close_ = [wSelf](){
                auto self = wSelf.lock();
                if (self) {
                    self->onError();
                }
            };
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
            // logInfo << "send rtmp msg";
            auto pktList = *(pack.get());
            for (auto& pkt : pktList) {
                // uint8_t frame_type = (pkt->payload.get()[0] >> 4) & 0x0f;
				// uint8_t codec_id = pkt->payload.get()[0] & 0x0f;

                self->_totalSendBytes += pkt->length;
                self->_intervalSendBytes += pkt->length;

                // logInfo << "send rtmp msg,time: " << pkt->abs_timestamp << ", type: " << (int)(pkt->type_id)
                //             << ", length: " << pkt->length;
                self->sendRtmpChunks(pkt->csid, *pkt);
                
                if (self->_addMute) {
                    // aac 一帧1024字节，采样率8000。一帧的时长，单位ms
                    static int scale = (1024 * 1000 / 8000);
                    int curMuteId = pkt->abs_timestamp / scale;
                    if (self->_lastMuteId != curMuteId) {
                        self->_lastMuteId = curMuteId;
                        auto buffer = AacFrame::getMuteForFlv();
                        RtmpMessage mute;
                        mute.payload = buffer;
                        mute.abs_timestamp = pkt->abs_timestamp;
                        mute.trackIndex_ = AudioTrackType;
                        mute.length = buffer->size();
                        mute.type_id = RTMP_AUDIO;
                        mute.csid = RTMP_CHUNK_AUDIO_ID;

                        self->_chunk.createChunk(mute.csid, mute);
                    }
                }
            }
        });

        PlayerInfo info;
        info.ip = _socket->getPeerIp();
        info.port = _socket->getPeerPort();
        info.protocol = PROTOCOL_RTMP;
        info.status = "on";
        info.type = _urlParser.type_;
        info.uri = _urlParser.path_;
        info.vhost = _urlParser.vhost_;

        MediaHook::instance()->onPlayer(info);
    }
}

bool RtmpConnection::handlePlay()
{
	//LOG_INFO("[Play] app: %s, stream name: %s, stream path: %s\n", app_.c_str(), stream_name_.c_str(), stream_path_.c_str());

	// auto server = rtmp_server_.lock();
	// if (!server) {
	// 	return false;
	// }

    // _urlParser.path_ = _path;
    // _urlParser.type_ = DEFAULT_TYPE;
    // _urlParser.protocol_ = "rtmp";
    // _urlParser.vhost_ = DEFAULT_VHOST;

    weak_ptr<RtmpConnection> wSelf = static_pointer_cast<RtmpConnection>(shared_from_this());
    MediaSource::getOrCreateAsync(_urlParser.path_, _urlParser.vhost_, _urlParser.protocol_, _urlParser.type_, 
    [wSelf](const MediaSource::Ptr &src){
        logInfo << "get a src";
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        self->_loop->async([wSelf, src](){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            self->responsePlay(src);
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
    
             
    // connection_state_ = START_PLAY; 
    
	// rtmp_session_ = server->GetSession(stream_path_);
	// auto session = rtmp_session_.lock();
    // if(session) {
	// 	session->AddSink(std::dynamic_pointer_cast<RtmpSink>(shared_from_this()));
    // }  
    
	// if (server) {
	// 	server->NotifyEvent("play.start", stream_path_);
	// }

    return true;
}

bool RtmpConnection::handlePlay2()
{
	handlePlay();
    //printf("[Play2] stream path: %s\n", stream_path_.c_str());
    return false;
}

bool RtmpConnection::handleDeleteStream()
{
	// auto server = rtmp_server_.lock();
	// if (!server) {
	// 	return false;
	// }

    if(_path != "") {
        // auto session = rtmp_session_.lock();
        // if(session) {   
		// 	auto conn = std::dynamic_pointer_cast<RtmpSink>(shared_from_this());
		// 	task_scheduler_->AddTimer([session, conn] {
		// 		session->RemoveSink(conn);
		// 		return false;
		// 	}, 1);

		// 	if (is_publishing_) {
		// 		server->NotifyEvent("publish.stop", stream_path_);
		// 	}
		// 	else if (is_playing_) {
		// 		server->NotifyEvent("play.stop", stream_path_);
		// 	}
        // }  

		// is_playing_ = false;
		// is_publishing_ = false;
		// has_key_frame_ = false;
		_chunk.clear();
    }

	return true;
}

bool RtmpConnection::sendMetaData(AmfObjects meta_data)
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
    if(!this->sendNotifyMessage(RTMP_CHUNK_DATA_ID, _amfEncoder.data(), _amfEncoder.size())) {
        return false;
    }

    return true;
}

void RtmpConnection::setPeerBandwidth()
{
    StreamBuffer::Ptr data = make_shared<StreamBuffer>(6);
    writeUint32BE(data->data(), 5000000);
    data->data()[4] = 2;
    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_BANDWIDTH_SIZE;
    rtmp_msg.payload = data;
    rtmp_msg.length = 5;
    sendRtmpChunks(RTMP_CHUNK_CONTROL_ID, rtmp_msg);
}

void RtmpConnection::sendAcknowledgement()
{
    StreamBuffer::Ptr data = make_shared<StreamBuffer>(5);
    writeUint32BE(data->data(), 5000000);

    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_ACK_SIZE;
    rtmp_msg.payload = data;
    rtmp_msg.length = 4;
    sendRtmpChunks(RTMP_CHUNK_CONTROL_ID, rtmp_msg);
}

void RtmpConnection::setChunkSize()
{
    // uint32_t chunkSize = _chunk.getInChunkSize() == 0 ? 5000000 : _chunk.getInChunkSize();
    // 设置4Mb，一般情况下，一帧没有这么大
    uint32_t chunkSize = 60000;
	_chunk.setOutChunkSize(chunkSize);
    StreamBuffer::Ptr data = make_shared<StreamBuffer>(5);
    writeUint32BE((char*)data->data(), chunkSize);

    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_SET_CHUNK_SIZE;
    rtmp_msg.payload = data;
    rtmp_msg.length = 4;
    sendRtmpChunks(RTMP_CHUNK_CONTROL_ID, rtmp_msg);
}

bool RtmpConnection::sendInvokeMessage(uint32_t csid, const StreamBuffer::Ptr& payload, uint32_t payload_size)
{
    if(!payload || payload_size == 0) {
        return false;
    }

    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_INVOKE;
    rtmp_msg.timestamp = 0;
    rtmp_msg.stream_id = _streamId;
    rtmp_msg.payload = payload;
    rtmp_msg.length = payload_size; 
    sendRtmpChunks(csid, rtmp_msg);  
    return true;
}

bool RtmpConnection::sendNotifyMessage(uint32_t csid, const StreamBuffer::Ptr& payload, uint32_t payload_size)
{
    if(!payload || payload_size == 0) {
        return false;
    }

    RtmpMessage rtmp_msg;
    rtmp_msg.type_id = RTMP_NOTIFY;
    rtmp_msg.timestamp = 0;
    rtmp_msg.stream_id = _streamId;
    rtmp_msg.payload = payload;
    rtmp_msg.length = payload_size; 
    sendRtmpChunks(csid, rtmp_msg);  
    return true;
}

bool RtmpConnection::isKeyFrame(const StreamBuffer::Ptr& payload, uint32_t payload_size)
{
    if(!payload || payload_size < 1) {
        return false;
    }

	uint8_t frame_type = (payload->data()[0] >> 4) & 0x0f;
	uint8_t codec_id = payload->data()[0] & 0x0f;
	return (frame_type == 1 && (codec_id == RTMP_CODEC_ID_H264 || codec_id == RTMP_CODEC_ID_H265));
}

void RtmpConnection::sendRtmpChunks(uint32_t csid, RtmpMessage& rtmp_msg)
{    
    // auto msg = rtmp_msg;
	_chunk.createChunk(csid, rtmp_msg);
}


