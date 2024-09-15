#include "HttpClient.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"

using namespace std;

HttpClient::HttpClient(const EventLoop::Ptr& loop)
    :TcpClient(loop)
    ,_loop(loop)
{}

HttpClient::HttpClient(const EventLoop::Ptr& loop, bool enableTls)
    :TcpClient(loop, enableTls)
    ,_loop(loop)
{}

HttpClient::~HttpClient()
{}

int HttpClient::start(const string& localIp, int localPort, const string& peerIp, int peerPort, int timeout)
{
    weak_ptr<HttpClient> wSelf = static_pointer_cast<HttpClient>(shared_from_this());
    logInfo << this;
    _parser.setOnHttpRequest([wSelf](){
        // logInfo << "HttpClient setOnHttpRequest";
        auto self = wSelf.lock();
        if (self) {
            self->onHttpRequest();
        }
    });

    _parser.setOnHttpBody([wSelf](const char* data, int len){
        // logInfo << "HttpClient setOnHttpBody";
        auto self = wSelf.lock();
        if (self) {
            self->onRecvContent(data, len);
        }
    });

    if (TcpClient::create(localIp, localPort) < 0) {
        close();
        logInfo << "TcpClient::create failed: " << strerror(errno);
        return -1;
    }

    if (TcpClient::connect(peerIp, peerPort, timeout) < 0) {
        close();
        logInfo << "TcpClient::connect, ip: " << peerIp << ", peerPort: " 
                << peerPort << ", failed: " << strerror(errno);
        return -1;
    }

    return 0;
}

int HttpClient::sendHeader(const string& url, int timeout)
{
    logInfo << "url: " << url;
    _urlParser.parse(url);

    if (_urlParser.port_ == 0) {
        if (_urlParser.protocol_ == "http") {
            _urlParser.port_ = 80;
        } else if (_urlParser.protocol_ == "https") {
            _urlParser.port_ = 443;
        } else {
            onError("invalid protocol: " + _urlParser.protocol_);
        }
    }

    return start("", 0, _urlParser.host_, _urlParser.port_, timeout);
}

int HttpClient::sendHeader(const string& localIp, int localPort, const string& url, int timeout)
{
    logInfo << "url: " << url;
    _urlParser.parse(url);

    if (_urlParser.port_ == 0) {
        if (_urlParser.protocol_ == "http") {
            _urlParser.port_ = 80;
        } else if (_urlParser.protocol_ == "https") {
            _urlParser.port_ = 443;
        } else {
            onError("invalid protocol: " + _urlParser.protocol_);
        }
    }
    return start(localIp, localPort, _urlParser.host_, _urlParser.port_, timeout);
}

void HttpClient::onConnect()
{
    // logInfo << "HttpClient::onConnect";
    stringstream ss;
    if (_request._method.empty()) {
        _request._method = "GET";
    }
    ss << _request._method << " " << (_urlParser.path_ + "?" + _urlParser.param_) << " HTTP/1.1\r\n"
       << "Host: " << _urlParser.host_ << "\r\n"
       << "Tools: SimpleMediaServer\r\n"
       << "Accept: */*\r\n"
       << "Accept-Language: zh-CN,zh;q=0.8\r\n";
    
    if (_request._mapHeaders.find("Connection") == _request._mapHeaders.end()) {
        ss << "Connection: close\r\n";
    } else {
        ss << "Connection: " << _request._mapHeaders["Connection"] << "\r\n";
    }
    if (!_request._content.empty()) {
        ss << "Content-Length: " << to_string(_request._content.size()) << "\r\n";
        if (_request._mapHeaders.find("Content-Type") == _request._mapHeaders.end()) {
            ss << "Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n";
        } else {
            ss << "Content-Type: " << _request._mapHeaders["Content-Type"] << "\r\n";
        }
    }
    if (_isWebsocket) {
        _websocketKey = "x3JJHMbDL1EzLkh9GBhXDw==";
        ss << "Upgrade: websocket\r\n"
           << "Connection: Upgrade\r\n"
           << "Sec-WebSocket-Key: " << _websocketKey << "\r\n"
           << "Sec-WebSocket-Protocol: chat, superchat\r\n"
           << "Sec-WebSocket-Version: 13\r\n";
    }
    ss << "\r\n";
    send(ss.str());
}

void HttpClient::sendContent(const char* data, int len)
{
    auto buffer = StreamBuffer::create();
    buffer->assign(data, len);
    TcpClient::send(buffer);
}

void HttpClient::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    _parser.parse(buffer->data(), buffer->size());
}

void HttpClient::onError(const string& err)
{
    close();
}

void HttpClient::close()
{
    TcpClient::close();
}

void HttpClient::onHttpRequest()
{
    // if (_parser._url == "302" || _parser._url == "301") {

    // }

    // if (_parser._mapHeaders.find("Content-Length") != _parser._mapHeaders.end())
    // {
    //     _totalBodySize = atoll(_parser._mapHeaders["Content-Length"].data());
    // } else {
    //     _totalBodySize = getBodySize();
    // }

    // if(_parser["Transfer-Encoding"] == "chunked"){
    //     //如果Transfer-Encoding字段等于chunked，则认为后续的content是不限制长度的
    //     _totalBodySize = -1;
    //     _chunkedSplitter = std::make_shared<HttpChunkedSplitter>([this](const char *data,uint64_t len){
    //         if(len > 0){
    //             auto recvedBodySize = _recvedBodySize + len;
    //             onResponseBody(data, len, recvedBodySize, INT64_MAX);
    //             _recvedBodySize = recvedBodySize;
    //         }else{
    //             onResponseCompleted_l();
    //         }
    //     });
    // }

    // if(_totalBodySize == 0){
    //     //后续没content，本次http请求结束
    //     onResponseCompleted_l();
    //     return 0;
    // }

    // _recvedBodySize = 0;
}

void HttpClient::onRecvContent(const char *data, uint64_t len)
{
    // if(_chunkedSplitter){
    //     _chunkedSplitter->input(data,len);
    //     return;
    // }
    // auto recvedBodySize = _recvedBodySize + len;
    // if(_totalBodySize < 0){
    //     //不限长度的content,最大支持INT64_MAX个字节
    //     onResponseBody(data, len, recvedBodySize, INT64_MAX);
    //     _recvedBodySize = recvedBodySize;
    //     return;
    // }

    // //固定长度的content
    // if ( recvedBodySize < _totalBodySize ) {
    //     //content还未接收完毕
    //     onResponseBody(data, len, recvedBodySize, _totalBodySize);
    //     _recvedBodySize = recvedBodySize;
    //     return;
    // }

    // //content接收完毕
    // onResponseBody(data, _totalBodySize - _recvedBodySize, _totalBodySize, _totalBodySize);
    // bool biggerThanExpected = recvedBodySize > _totalBodySize;
    // onResponseCompleted_l();
    // if(biggerThanExpected) {
    //     //声明的content数据比真实的小，那么我们只截取前面部分的并断开链接
    //     shutdown(SockException(Err_shutdown, "http response content size bigger than expected"));
    // }
}

void HttpClient::addHeader(const string& key, const string& header)
{
    _request._mapHeaders[key] = header;
}

void HttpClient::setContent(const string& content)
{
    _request._content = content;
    _request._contentLen = content.size();
}
    
void HttpClient::setMethod(const string& method)
{
    _request._method = method;
}

void HttpClient::send(const string& msg)
{
    auto buffer = StreamBuffer::create();
    buffer->assign(msg.data(), msg.size());
    TcpClient::send(buffer);
}