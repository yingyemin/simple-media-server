#ifndef WebsocketConnection_h
#define WebsocketConnection_h

#include "HttpConnection.h"
#include "Common/FrameMediaSource.h"

#include <string>
#include <memory>
#include <functional>

using namespace std;

class WebsocketConnection : public HttpConnection
{
public:
    WebsocketConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket, bool enbaleSsl);
    ~WebsocketConnection();

public:
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onWebsocketFrame(const char* data, int len) override;
    void handleGet() override;

private:
    bool _handshake = false;
    bool _first = true;
    bool _enableSsl;
    // UrlParser _urlParser;
    // EventLoop::Ptr _loop;
    // Socket::Ptr _socket;
    FrameMediaSource::Wptr _source;
};

#endif //WebsocketConnection_h