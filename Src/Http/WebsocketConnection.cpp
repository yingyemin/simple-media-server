#include "WebsocketConnection.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Common/Define.h"
#include "Util/String.h"
#include "Util/Base64.h"
#include "Ssl/SHA1.h"
#include "Codec/G711Track.h"

using namespace std;
WebsocketConnection::WebsocketConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket, bool enableSsl)
    :HttpConnection(loop, socket, enableSsl)
    ,_enableSsl(enableSsl)
{}

WebsocketConnection::~WebsocketConnection()
{
    auto frameSrc = _source.lock();
    if (frameSrc) {
        frameSrc->release();
    }
}

void WebsocketConnection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    logInfo << "get a buf: " << buffer->size();
    if (!_handshake) {
        _parser.parse(buffer->data(), buffer->size());
    } else {
        _onHttpBody(buffer->data(), buffer->size());
    }
}

void WebsocketConnection::onWebsocketFrame(const char* data, int len)
{
    logInfo << "get websocket msg: " << string(data, len);
    if (_first) {
        _urlParser.path_.assign(data, len);
        _urlParser.protocol_= "websocket";
        _urlParser.type_ = DEFAULT_TYPE;
        _urlParser.vhost_ = DEFAULT_VHOST;

        auto source = MediaSource::getOrCreate(_urlParser.path_, _urlParser.vhost_, _urlParser.protocol_, _urlParser.type_, 
        [this](){
            return make_shared<FrameMediaSource>(_urlParser, _loop);
        });

        if (!source) {
            logWarn << "another stream is exist with the same uri";
            return ;
        }

        logInfo << "create a FrameMediaSource";
        auto frameSrc = dynamic_pointer_cast<FrameMediaSource>(source);
        if (!frameSrc) {
            logWarn << "source is not gb source";
            return ;
        }
        frameSrc->setOrigin();
        _source = frameSrc;

        auto trackInfo = G711aTrack::createTrack(AudioTrackType, 8, 8000);
        frameSrc->addTrack(trackInfo);
        frameSrc->onReady();

        _first = false;
        return ;
    }

    // wavToG711a
    auto frame = FrameBuffer::createFrame("g711a", 0, AudioTrackType, false);
    frame->_buffer.assign((char*)data, len);
    frame->_pts = frame->_dts = 0;

    auto frameSrc = _source.lock();
    if (!frameSrc) {
        return ;
    }

    frameSrc->onFrame(frame);
}

void WebsocketConnection::handleGet()
{
    weak_ptr<WebsocketConnection> wSelf = static_pointer_cast<WebsocketConnection>(shared_from_this());
    if (_parser._mapHeaders.find("sec-websocket-key") != _parser._mapHeaders.end()) {
        _isWebsocket = true;
        _websocket.setOnWebsocketFrame([wSelf](const char* data, int len){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            self->onWebsocketFrame(data, len);
        });

        _onHttpBody = [wSelf](const char* data, int len){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            self->_websocket.decode((char*)data, len);
        };
    }

    _parser._contentLen = 0;

    HttpResponse rsp;
    rsp._status = 200;
    // json value;
    // value["msg"] = "success";
    // rsp.setContent(value.dump());
    writeHttpResponse(rsp);

    _handshake = true;
}
