#include "HttpClientApi.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"

using namespace std;

HttpClientApi::HttpClientApi(const EventLoop::Ptr& loop)
    :HttpClient(loop)
    ,_loop(loop)
{}

HttpClientApi::HttpClientApi(const EventLoop::Ptr& loop, bool enableTls)
    :HttpClient(loop, enableTls)
    ,_loop(loop)
{}

HttpClientApi::~HttpClientApi()
{}

void HttpClientApi::onHttpRequest()
{
    if(_parser._contentLen == 0){
        //后续没content，本次http请求结束
        HttpClient::close();
    }
}

void HttpClientApi::onConnect()
{
    HttpClient::onConnect();
    sendContent(_request._content.data(), _request._content.size());
}

void HttpClientApi::onRecvContent(const char *data, uint64_t len) {
    if (len == 0 && _parser._contentLen == 0) {
        HttpClient::close();
        onHttpResponce();
    }else if (_parser._contentLen == 0) {
        _parser._content.append(data, len);
        HttpClient::close();
        onHttpResponce();
    // } else if (_parser._contentLen < len) {
    //     logInfo << "recv body len(" << len << ") is bigger than conten-length(" << _parser._contentLen << ")";
    //     _parser._content.append(data, _parser._contentLen);
    //     HttpClient::close();
    //     onHttpResponce();
    } else {
        _parser._content.append(data, len);
    }
}

void HttpClientApi::setOnHttpResponce(const function<void(const HttpParser& parser)>& cb)
{
    _onHttpResponce = cb;
}

void HttpClientApi::onHttpResponce()
{
    if (_onHttpResponce) {
        _onHttpResponce(_parser);
    }
}

void HttpClientApi::onError(const string& err)
{
    onHttpResponce();
    HttpClient::onError(err);
}
