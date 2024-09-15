#include "WebsocketClient.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "Util/Base64.h"
#include "Ssl/SHA1.h"

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
}

void WebsocketClient::onConnect()
{
    HttpClient::onConnect();
    // sendContent(_request._content.data(), _request._content.size());
    if (_onSendData) {
        _onSendData();
    }
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

void WebsocketClient::onRecvContent(const char *data, uint64_t len) {
    _recvLen += len;
    if (len == 0) {
        HttpClient::close();
    }else if (_parser._contentLen == _recvLen) {
        _websocket.decode((char*)data, len);
        HttpClient::close();
    } else if (_parser._contentLen < _recvLen) {
        logInfo << "recv body len is bigger than conten-length";
        _websocket.decode((char*)data, len);
        HttpClient::close();
    } else {
        _websocket.decode((char*)data, len);
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
