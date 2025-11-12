#ifndef HttpClientApi_h
#define HttpClientApi_h

#include "HttpParser.h"
#include "Common/UrlParser.h"
#include "HttpClient.h"
#include "Http/HttpChunkedParser.h"

#include <string>
#include <memory>
#include <functional>

// using namespace std;

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
    void onError(const std::string& err) override;

    void onHttpResponce();
    void setOnHttpResponce(const std::function<void(const HttpParser& parser)>& cb);

    static void reportByHttp(const std::string& url, const std::string&method, const std::string& msg, const std::function<void(const std::string& err, 
                const nlohmann::json& res)>& cb = [](const std::string& err, const nlohmann::json& res){});

private:
    EventLoop::Ptr _loop;
    HttpChunkedParser::Ptr _chunkedParser;
    std::function<void(const HttpParser& parser)> _onHttpResponce;
};

#endif //HttpClient_h