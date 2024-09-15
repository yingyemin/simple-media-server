#ifndef WebsocketClient_h
#define WebsocketClient_h

#include "HttpParser.h"
#include "Common/UrlParser.h"
#include "HttpClient.h"
#include "WebsocketContext.h"

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

    void onWebsocketFrame(const char *data, uint64_t len);
    void setOnWebsocketFrame(const function<void(const char *data, uint64_t len)>& cb);
    void sendData(const Buffer::Ptr& buffer);

private:
    uint64_t _recvLen = 0;
    WebsocketContext _websocket;
    EventLoop::Ptr _loop;
    function<void(const HttpParser& parser)> _onHttpResponce;
    function<void()> _onSendData;
    function<void(const char *data, uint64_t len)> _onWebsocketFrame;
};

#endif //WebsocketClient_h