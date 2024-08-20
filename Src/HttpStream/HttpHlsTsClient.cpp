#include "HttpHlsTsClient.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "Common/Define.h"

using namespace std;

HttpHlsTsClient::HttpHlsTsClient()
    :HttpClient(EventLoop::getCurrentLoop())
{
}

HttpHlsTsClient::~HttpHlsTsClient()
{}

void HttpHlsTsClient::start(const string& localIp, int localPort, const string& url, int timeout)
{
    weak_ptr<HttpHlsTsClient> wSelf = dynamic_pointer_cast<HttpHlsTsClient>(shared_from_this());

    addHeader("Content-Type", "application/json;charset=UTF-8");
    setMethod("GET");
    // setContent(msg);
    // setOnHttpResponce([client, cb](const HttpParser &parser){
    //     // logInfo << "uri: " << parser._url;
    //     // logInfo << "status: " << parser._version;
    //     // logInfo << "method: " << parser._method;
    //     // logInfo << "_content: " << parser._content;
    //     if (parser._url != "200") {
    //         cb("http error", "");
    //     }
    //     try {
    //         json value = json::parse(parser._content);
    //         cb("", value);
    //     } catch (exception& ex) {
    //         logInfo << "json parse failed: " << ex.what();
    //         cb(ex.what(), nullptr);
    //     }

    //     const_cast<shared_ptr<HttpClientApi> &>(client).reset();
    // });
    logInfo << "connect to utl: " << url;
    if (sendHeader(url, timeout) != 0) {
        close();
    }
}

void HttpHlsTsClient::onHttpRequest()
{
    if(_parser._contentLen == 0){
        //后续没content，本次http请求结束
        close();
    }
}

void HttpHlsTsClient::onConnect()
{
    _socket = TcpClient::getSocket();
    _loop = _socket->getLoop();

    HttpClient::onConnect();
    sendContent(_request._content.data(), _request._content.size());

    // auto source = MediaSource::getOrCreate(_localUrlParser.path_, DEFAULT_VHOST
    //                     , PROTOCOL_RTMP, DEFAULT_TYPE
    //                     , [this](){
    //                         return make_shared<RtmpMediaSource>(_localUrlParser);
    //                     });

    // if (!source) {
    //     logWarn << "another source is exist: " << _localUrlParser.path_;
    //     onError("another source is exist");

    //     return ;
    // }

    // auto rtmpSrc = dynamic_pointer_cast<RtmpMediaSource>(source);
    // if (!rtmpSrc) {
    //     logWarn << "this source is not rtmp: " << source->getProtocol();
    //     onError("this source is not rtmp");

    //     return ;
    // }

    // _source = rtmpSrc;
}

void HttpHlsTsClient::onRecvContent(const char *data, uint64_t len)
{
    logInfo << "recv content: " << len;
    onTsPacket(data, len);
}

void HttpHlsTsClient::close()
{
    if (_onClose) {
        _onClose();
    }

    HttpClient::close();
}

void HttpHlsTsClient::onError(const string& err)
{
    logInfo << "get a error: " << err;

    close();
}

void HttpHlsTsClient::setOnTsPacket(const function<void(const char* tsPacket, int len)>& cb)
{
    _onTsPacket = cb;
}

void HttpHlsTsClient::onTsPacket(const char* tsPacket, int len)
{
    if (_onTsPacket) {
        _onTsPacket(tsPacket, len);
    }
}