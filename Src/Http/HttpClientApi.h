#ifndef HttpClientApi_h
#define HttpClientApi_h

#include "HttpParser.h"
#include "Common/UrlParser.h"
#include "HttpClient.h"

#include <string>
#include <memory>
#include <functional>

using namespace std;

class HttpClientApi : public HttpClient
{
public:
    HttpClientApi(const EventLoop::Ptr& loop);
    HttpClientApi(const EventLoop::Ptr& loop, bool enableTls);
    ~HttpClientApi();

public:
    void onHttpRequest() override;
    void onRecvContent(const char *data, uint64_t len) override;
    void onConnect() override;

    void onHttpResponce();
    void setOnHttpResponce(const function<void(const HttpParser& parser)>& cb);

private:
    EventLoop::Ptr _loop;
    function<void(const HttpParser& parser)> _onHttpResponce;
};

#endif //HttpClient_h