﻿#include "HttpClientApi.h"
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

void HttpClientApi::reportByHttp(const string& url, const string&method, const string& msg, const function<void(const string& err, const json& res)>& cb)
{
    static int timeout = Config::instance()->getAndListen([](const json& config){
        timeout = Config::instance()->get("Hook", "Http", "timeout");
    }, "Hook", "Http", "timeout");

    if (url.empty()) {
        return ;
    }
    
    shared_ptr<HttpClientApi> client;
    if (startWith(url, "https://")) {
        client = make_shared<HttpClientApi>(EventLoop::getCurrentLoop(), true);
    } else {
        client = make_shared<HttpClientApi>(EventLoop::getCurrentLoop());
    }
    client->addHeader("Content-Type", "application/json;charset=UTF-8");
    client->setMethod(method);
    client->setContent(msg);
    client->setOnHttpResponce([url, client, cb](const HttpParser &parser){
        // logInfo << "uri: " << parser._url;
        // logInfo << "status: " << parser._version;
        // logInfo << "method: " << parser._method;
        // logInfo << "_content: " << parser._content;

        logInfo << "client: " << client << ", url: " << url << ", response: " << parser._content;
        if (parser._url != "200") {
            cb("http error", "");
        }
        try {
            json value = json::parse(parser._content);
            cb("", value);
        } catch (exception& ex) {
            logInfo << url << ", json parse failed: " << ex.what();
            cb(ex.what(), nullptr);
        }

        const_cast<shared_ptr<HttpClientApi> &>(client).reset();
    });
    logInfo << "connect to url: " << url << ", body: " << msg << ", client: " << client;
    if (client->sendHeader(url, timeout) != 0) {
        cb("connect to url: " + url + " failed", nullptr);
    }
}
