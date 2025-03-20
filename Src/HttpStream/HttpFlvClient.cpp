#ifdef ENABLE_RTMP

#include "HttpFlvClient.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "Common/Define.h"
#include "Rtmp/Amf.h"
#include "Rtmp/Rtmp.h"

using namespace std;

HttpFlvClient::HttpFlvClient(MediaClientType type, const string& appName, const string& streamName)
    :HttpClient(EventLoop::getCurrentLoop())
    ,_type(type)
{
    _localUrlParser.path_ = "/" + appName + "/" + streamName;
    _localUrlParser.protocol_ = PROTOCOL_RTMP;
    _localUrlParser.type_ = DEFAULT_TYPE;
    _localUrlParser.vhost_ = DEFAULT_VHOST;
}

HttpFlvClient::~HttpFlvClient()
{
    auto rtmpSrc = _source.lock();
    if (rtmpSrc) {
        rtmpSrc->release();
        rtmpSrc->delConnection(this);
    }
}

bool HttpFlvClient::start(const string& localIp, int localPort, const string& url, int timeout)
{
    weak_ptr<HttpFlvClient> wSelf = dynamic_pointer_cast<HttpFlvClient>(shared_from_this());
    _demuxer.setOnError([wSelf](const string& err){
        auto self = wSelf.lock();
        if (self) {
            self->onError(err);
        }
    });

    _demuxer.setOnFlvHeader([wSelf](const char* data, int len){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        self->_hasVideo = data[4] & 0x1;
        self->_hasAudio = data[4] & 0x4;
    });

    _demuxer.setOnFlvMetadata([wSelf](const char* data, int len){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        // 去掉flv tag头
        data += 11;
        len -= 11;

        AmfDecoder decoder;
        int bytes_used = 0;
        bytes_used += decoder.decode(data + bytes_used, len - bytes_used, 1);
        if (decoder.getString() != "onMetaData") {
            logWarn << "flv metadata header error: " << decoder.getString();
            return ;
        }

        decoder.reset();
        bytes_used += decoder.decode(data + bytes_used, len - bytes_used, 1);
        auto src = self->_source.lock();
        if (src) {
            src->setMetadata(decoder.getObjects());
        }
    });

    _demuxer.setOnFlvMedia([wSelf](const char* data, int len){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        auto type = data[0];

        // 去掉flv tag头
        // data += 11;
        // len -= 11;

        if (type == 8) {
            self->handleAudio(data, len);
        } else if (type == 9) {
            self->handleVideo(data, len);
        } else {
            logWarn << "invalid tag type: " << (int)(type);
        }
    });


    addHeader("Content-Type", "application/json;charset=UTF-8");
    setMethod("GET");
    // setContent(msg);
    // setOnHttpResponce([client, cb](const HttpParser &parser){
    //     // logInfo << "uri: " << parser._url;
    //     // logInfo << "status: " << parser._version;
    //     // logInfo << "method: " << parser._method;
    //     // logInfo << "_content: " << parser._content;
    //     if (parser._url != "200") {
    //         cb("http error", "");
    //     }
    //     try {
    //         json value = json::parse(parser._content);
    //         cb("", value);
    //     } catch (exception& ex) {
    //         logInfo << "json parse failed: " << ex.what();
    //         cb(ex.what(), nullptr);
    //     }

    //     const_cast<shared_ptr<HttpClientApi> &>(client).reset();
    // });
    logInfo << "connect to utl: " << url;
    if (sendHeader(url, timeout) != 0) {
        close();
        return false;
    }

    return true;
}

void HttpFlvClient::stop()
{
    close();
}

void HttpFlvClient::pause()
{

}

void HttpFlvClient::onHttpRequest()
{
    // if(_parser._contentLen == 0){
    //     //后续没content，本次http请求结束
    //     close();
    // }

    if (_parser._mapHeaders["transfer-encoding"] == "chunked") {
        weak_ptr<HttpFlvClient> wSelf = dynamic_pointer_cast<HttpFlvClient>(shared_from_this());
        _chunkedParser = make_shared<HttpChunkedParser>();
        _chunkedParser->setOnHttpBody([wSelf](const char *data, int len){
            auto self = wSelf.lock();
            if (self) {
                self->_demuxer.input(data, len);
            }
        });
    }
}

void HttpFlvClient::onConnect()
{
    _socket = TcpClient::getSocket();
    _loop = _socket->getLoop();

    HttpClient::onConnect();
    sendContent(_request._content.data(), _request._content.size());

    auto source = MediaSource::getOrCreate(_localUrlParser.path_, DEFAULT_VHOST
                        , PROTOCOL_RTMP, DEFAULT_TYPE
                        , [this](){
                            return make_shared<RtmpMediaSource>(_localUrlParser, _loop);
                        });

    if (!source) {
        logWarn << "another source is exist: " << _localUrlParser.path_;
        onError("another source is exist");

        return ;
    }

    auto rtmpSrc = dynamic_pointer_cast<RtmpMediaSource>(source);
    if (!rtmpSrc) {
        logWarn << "this source is not rtmp: " << source->getProtocol();
        onError("this source is not rtmp");

        return ;
    }
    rtmpSrc->setOrigin();
    rtmpSrc->setOriginSocket(_socket);
    rtmpSrc->setAction(false);

    _source = rtmpSrc;
}

void HttpFlvClient::onRecvContent(const char *data, uint64_t len) {
    // if (len == 0) {
    //     close();
    //     onHttpResponce();
    // }else if (_parser._contentLen == len) {
    //     _parser._content.append(data, len);
    //     close();
    //     onHttpResponce();
    // } else if (_parser._contentLen < len) {
    //     logInfo << "recv body len is bigger than conten-length";
    //     _parser._content.append(data, _parser._contentLen);
    //     close();
    //     onHttpResponce();
    // } else {
    //     _parser._content.append(data, len);
    // }

    if (_chunkedParser) {
        _chunkedParser->parse(data, len);
        return ;
    }

    _demuxer.input(data, len);
}

void HttpFlvClient::close()
{
    if (_onClose) {
        _onClose();
    }

    HttpClient::close();
}

void HttpFlvClient::onError(const string& err)
{
    logInfo << "get a error: " << err;

    close();
}

void HttpFlvClient::setOnHttpResponce(const function<void(const HttpParser& parser)>& cb)
{
    _onHttpResponce = cb;
}

void HttpFlvClient::onHttpResponce()
{
    if (_onHttpResponce) {
        _onHttpResponce(_parser);
    }
}

void HttpFlvClient::setOnClose(const function<void()>& cb)
{
    _onClose = cb;
}

void HttpFlvClient::handleVideo(const char* data, int len)
{
    if (!_validVideoTrack) {
        return ;
    }
    auto rtmpSrc = _source.lock();
    if (!rtmpSrc) {
        onError("source is empty");
        return ;
    }
	uint8_t type = RTMP_VIDEO;
	uint8_t *payload = (u_char*)data + 11;
	uint32_t length = len - 11;
	uint8_t frame_type = (payload[0] >> 4) & 0x0f;
	uint8_t codec_id = payload[0] & 0x0f;

    uint32_t timestamp = readUint24BE(data + 4); //扩展字段也读了

    logInfo << "timestamp: " << timestamp;
    timestamp |= ((data[8]) << 24);

    if (!_rtmpVideoDecodeTrack) {
        _rtmpVideoDecodeTrack = make_shared<RtmpDecodeTrack>(VideoTrackType);
        if (_rtmpVideoDecodeTrack->createTrackInfo(VideoTrackType, codec_id) != 0) {
            _validVideoTrack = false;
            return ;
        }
        rtmpSrc->addTrack(_rtmpVideoDecodeTrack);
    }

    auto msg = make_shared<RtmpMessage>();
    msg->payload = make_shared<StreamBuffer>(length + 1);
    memcpy(msg->payload->data(), payload, length);
    msg->abs_timestamp = timestamp;
    msg->length = length;
    msg->type_id = RTMP_VIDEO;
    msg->csid = RTMP_CHUNK_VIDEO_ID;

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

    // return true;
}

void HttpFlvClient::handleAudio(const char* data, int len)
{
    if (!_validAudioTrack) {
        return ;
    }
    auto rtmpSrc = _source.lock();
    if (!rtmpSrc) {
        close();
        return ;
    }
	uint8_t type = RTMP_AUDIO;
	uint8_t *payload = (u_char*)data + 11;
	uint32_t length = len - 11;
	uint8_t sound_format = (payload[0] >> 4) & 0x0f;
	//uint8_t sound_size = (payload[0] >> 1) & 0x01;
	//uint8_t sound_rate = (payload[0] >> 2) & 0x03;
	uint8_t codec_id = payload[0] & 0x0f;

    uint32_t timestamp = readUint32BE(data + 4); //扩展字段也读了

    if (!_rtmpAudioDecodeTrack) {
        _rtmpAudioDecodeTrack = make_shared<RtmpDecodeTrack>(AudioTrackType);
        if (_rtmpAudioDecodeTrack->createTrackInfo(AudioTrackType, sound_format) != 0) {
            _validAudioTrack = false;
            return ;
        }
        rtmpSrc->addTrack(_rtmpAudioDecodeTrack);
    }

    auto msg = make_shared<RtmpMessage>();
    msg->payload = make_shared<StreamBuffer>(length + 1);
    
    memcpy(msg->payload.get(), payload, length);

    msg->abs_timestamp = timestamp;
    msg->length = length;
    msg->type_id = RTMP_AUDIO;
    msg->csid = RTMP_CHUNK_AUDIO_ID;

    // auto msg = make_shared<RtmpMessage>(std::move(msg));
    if (!_aacHeader && sound_format == RTMP_CODEC_ID_AAC && payload[1] == 0) {
        _aacHeaderSize = msg->length;
        _aacHeader = make_shared<StreamBuffer>(length + 1);
        memcpy(_aacHeader->data(), msg->payload->data(), msg->length);
        // session->SetAacSequenceHeader(aac_sequence_header_, aac_sequence_header_size_);
        type = RTMP_AAC_SEQUENCE_HEADER;
        rtmpSrc->setAacHeader(_aacHeader, _aacHeaderSize);
        _rtmpAudioDecodeTrack->setConfigFrame(msg);
    }

    msg->trackIndex_ = AudioTrackType;
    _rtmpAudioDecodeTrack->onRtmpPacket(msg);

    // session->SendMediaData(type, rtmp_msg._timestamp, rtmp_msg.payload, rtmp_msg.length);

    // return ;
}

#endif