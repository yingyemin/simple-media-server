#ifndef WebsocketClient_h
#define WebsocketClient_h

#include "HttpParser.h"
#include "Common/UrlParser.h"
#include "HttpClient.h"
#include "WebsocketContext.h"
#include "Common/FrameMediaSource.h"

#include <string>
#include <memory>
#include <functional>

using namespace std;

class WebsocketClient : public HttpClient
{
public:
    WebsocketClient(const EventLoop::Ptr& loop);
    WebsocketClient(const EventLoop::Ptr& loop, bool enableTls);
    ~WebsocketClient();

public:
    void onHttpRequest() override;
    void onRecvContent(const char *data, uint64_t len) override;
    void onConnect() override;
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void close() override;

    void onWebsocketFrame(const char *data, uint64_t len);
    void setOnWebsocketFrame(const function<void(const char *data, uint64_t len)>& cb);
    void sendData(const Buffer::Ptr& buffer);
    void sendStream();
    void onGetSource(const MediaSource::Ptr& src);
    void setUri(const string& uri) {_uri = uri;}

    void setOnClose(const function<void()>& cb) {_onClose = cb;}

private:
    bool _handshake = false;
    uint64_t _recvLen = 0;
    string _uri;
    WebsocketContext _websocket;
    EventLoop::Ptr _loop;
    FrameMediaSource::Wptr _source;
    MediaSource::FrameRingType::DataQueReaderT::Ptr _playReader;
    function<void(const HttpParser& parser)> _onHttpResponce;
    function<void()> _onSendData;
    function<void()> _onClose;
    function<void(const char *data, uint64_t len)> _onWebsocketFrame;
};

#endif //WebsocketClient_h