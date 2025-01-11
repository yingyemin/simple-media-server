#include "WebsocketClient.h"
#include "Logger.h"
#include "Common/Config.h"
// #include "Util/String.h"
#include "Util/Base64.h"
#include "Ssl/SHA1.h"
#include "Common/Define.h"
#include "Hook/MediaHook.h"

using namespace std;

WebsocketClient::WebsocketClient(const EventLoop::Ptr& loop)
    :HttpClient(loop)
    ,_loop(loop)
{
    setWebsocket();
}

WebsocketClient::WebsocketClient(const EventLoop::Ptr& loop, bool enableTls)
    :HttpClient(loop, enableTls)
    ,_loop(loop)
{}

void WebsocketClient::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    logInfo << "get a buf: " << buffer->size();
    if (!_handshake) {
        _parser.parse(buffer->data(), buffer->size());
    } else {
        onRecvContent(buffer->data(), buffer->size());
    }
}

void WebsocketClient::close()
{
    HttpClient::close();
    if (_onClose) {
        _onClose();
    }
}

WebsocketClient::~WebsocketClient()
{}

void WebsocketClient::onHttpRequest()
{
    if (_parser._url != "101") {
        HttpClient::close();
        return ;
    }
    if (_parser._mapHeaders.find("sec-websocket-accept") != _parser._mapHeaders.end())
    {
        static string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        string acceptRecv = _parser._mapHeaders["sec-websocket-accept"];
        auto acceptStr = Base64::encode(SHA1::encode(_websocketKey + magic));
        if (acceptRecv != acceptStr) {
            HttpClient::close();
            return ;
        }
    } else {
        HttpClient::close();
        return ;
    }

    weak_ptr<WebsocketClient> wSelf = dynamic_pointer_cast<WebsocketClient>(shared_from_this());
    _websocket.setOnWebsocketFrame([wSelf](const char* data, int len){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        self->onWebsocketFrame(data, len);
    });
    _handshake = true;
    if (!_uri.empty()) {
        sendStream();
    }
}

void WebsocketClient::onConnect()
{
    HttpClient::onConnect();
    // sendContent(_request._content.data(), _request._content.size());
    // if (_onSendData) {
    //     _onSendData();
    // }
}

void WebsocketClient::sendData(const Buffer::Ptr& buffer)
{
    WebsocketFrame frame;
    frame.finish = 1;
    frame.rsv1 = 0;
    frame.rsv2 = 0;
    frame.rsv3 = 0;
    frame.opcode = OpcodeType_BINARY;
    frame.mask = 0;
    frame.payloadLen = buffer->size();

    auto header = make_shared<StringBuffer>();
    _websocket.encodeHeader(frame, header);

    TcpClient::send(header);
    TcpClient::send(buffer);
}

void WebsocketClient::sendStream()
{
    weak_ptr<WebsocketClient> wSelf = static_pointer_cast<WebsocketClient>(shared_from_this());
    UrlParser urlParser;
    urlParser.path_ = _uri;
    urlParser.vhost_ = DEFAULT_VHOST;
    urlParser.protocol_ = PROTOCOL_WEBSOCKET;
    urlParser.type_ = DEFAULT_TYPE;
    _uri = urlParser.path_;
    MediaSource::getOrCreateAsync(urlParser.path_, urlParser.vhost_, urlParser.protocol_, urlParser.type_, 
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

            self->onGetSource(src);
        }, true);
    }, 
    [wSelf, urlParser]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }
        auto source = make_shared<FrameMediaSource>(urlParser, nullptr);

        return source;
    }, this);
}

void WebsocketClient::onGetSource(const MediaSource::Ptr& src)
{
    if (!src) {
        close();
        return ;
    }

    auto frameSrc = dynamic_pointer_cast<FrameMediaSource>(src);
    if (!frameSrc) {
        close();
        return ;
    }

    _source = frameSrc;

    if (!_playReader) {
        logInfo << "set _playReader";
        weak_ptr<WebsocketClient> wSelf = static_pointer_cast<WebsocketClient>(shared_from_this());
        // static int interval = Config::instance()->getAndListen([](const json &config){
        //     interval = Config::instance()->get("Websocket", "Server", "Server1", "interval");
        //     if (interval == 0) {
        //         interval = 5000;
        //     }
        // }, "Websocket", "Server", "Server1", "interval");

        // if (interval == 0) {
        //     interval = 5000;
        // }
        // weak_ptr<RtmpConnection> wSelf = static_pointer_cast<RtmpConnection>(shared_from_this());
        // _loop->addTimerTask(interval, [wSelf](){
        //     auto self = wSelf.lock();
        //     if (!self) {
        //         return 0;
        //     }
        //     self->_lastBitrate = self->_intervalSendBytes / (interval / 1000.0);
        //     self->_intervalSendBytes = 0;

        //     return interval;
        // }, nullptr);

        _playReader = frameSrc->getRing()->attach(_loop, true);
        _playReader->setGetInfoCB([wSelf]() {
            auto self = wSelf.lock();
            ClientInfo ret;
            if (!self) {
                return ret;
            }
            ret.ip_ = self->getSocket()->getLocalIp();
            ret.port_ = self->getSocket()->getLocalPort();
            ret.protocol_ = PROTOCOL_WEBSOCKET;
            // ret.bitrate_ = self->_lastBitrate;
            ret.close_ = [wSelf](){
                auto self = wSelf.lock();
                if (self) {
                    self->onError("close");
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
        _playReader->setReadCB([wSelf](const MediaSource::FrameRingDataType &pack) {
            auto self = wSelf.lock();
            if (!self/* || pack->empty()*/) {
                return;
            }
            auto buffer = make_shared<StringBuffer>(pack->_buffer);
            self->sendData(buffer);
            
        });

        PlayerInfo info;
        info.ip = getSocket()->getPeerIp();
        info.port = getSocket()->getPeerPort();
        info.protocol = PROTOCOL_WEBSOCKET;
        info.status = "on";
        info.type = DEFAULT_TYPE;
        info.uri = _uri;
        info.vhost = DEFAULT_VHOST;

        MediaHook::instance()->onPlayer(info);
    }
}

void WebsocketClient::onRecvContent(const char *data, uint64_t len) {
    _recvLen += len;
    _handshake = true;
    if (!_uri.empty()) {
        _websocket.decode((unsigned char*)data, len);
        return ;
    }
    if (len == 0) {
        HttpClient::close();
    }else if (_parser._contentLen == _recvLen) {
        _websocket.decode((unsigned char*)data, len);
        HttpClient::close();
    } else if (_parser._contentLen < _recvLen) {
        logInfo << "recv body len is bigger than conten-length";
        _websocket.decode((unsigned char*)data, len);
        HttpClient::close();
    } else {
        _websocket.decode((unsigned char*)data, len);
    }
}

void WebsocketClient::setOnWebsocketFrame(const function<void(const char *data, uint64_t len)>& cb)
{
    _onWebsocketFrame = cb;
}

void WebsocketClient::onWebsocketFrame(const char *data, uint64_t len)
{
    if (_onWebsocketFrame) {
        _onWebsocketFrame(data, len);
    }
}
