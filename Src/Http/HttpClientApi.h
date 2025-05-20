#ifndef HttpClientApi_h
#define HttpClientApi_h

#include "HttpParser.h"
#include "Common/UrlParser.h"
#include "HttpClient.h"
#include "Http/HttpChunkedParser.h"

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
    void onError(const string& err) override;

    void onHttpResponce();
    void setOnHttpResponce(const function<void(const HttpParser& parser)>& cb);

    static void reportByHttp(const string& url, const string&method, const string& msg, const function<void(const string& err, 
                const nlohmann::json& res)>& cb = [](const string& err, const nlohmann::json& res){});

private:
    EventLoop::Ptr _loop;
    HttpChunkedParser::Ptr _chunkedParser;
    function<void(const HttpParser& parser)> _onHttpResponce;
};

#endif //HttpClient_h