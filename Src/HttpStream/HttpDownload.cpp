#include "HttpDownload.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "Common/Define.h"

using namespace std;

HttpDownload::HttpDownload()
    :HttpClient(EventLoop::getCurrentLoop())
{
}

HttpDownload::~HttpDownload()
{
}

void HttpDownload::setRange(uint64_t startPos, uint64_t size)
{
    uint64_t endPos = startPos + size - 1;
    addHeader("Range", "bytes=" + to_string(startPos) + "-" + to_string(endPos));
}

void HttpDownload::start(const string& localIp, int localPort, const string& url, int timeout)
{
    // addHeader("Content-Type", "application/json;charset=UTF-8");
    setMethod("GET");
    logInfo << "connect to utl: " << url;
    if (sendHeader(url, timeout) != 0) {
        close();
    }
}

void HttpDownload::stop()
{
    close();
}

void HttpDownload::pause()
{

}

void HttpDownload::onHttpRequest()
{
    // if(_parser._contentLen == 0){
    //     //后续没content，本次http请求结束
    //     close();
    // }

    auto rangeStr = _parser._mapHeaders["content-range"];
    if (!rangeStr.empty()) {
        auto vec = split(rangeStr, "/");
        if (vec.size() ==2) {
            _fileSize = stoull(vec[1]);
        }
    }

    if (_parser._mapHeaders["transfer-encoding"] == "chunked") {
        weak_ptr<HttpDownload> wSelf = dynamic_pointer_cast<HttpDownload>(shared_from_this());
        _chunkedParser = make_shared<HttpChunkedParser>();
        _chunkedParser->setOnHttpBody([wSelf](const char *data, int len){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            self->onHttpResponce(data, len);
        });
    }
}

void HttpDownload::onConnect()
{
    _socket = TcpClient::getSocket();
    _loop = _socket->getLoop();

    HttpClient::onConnect();
    sendContent(_request._content.data(), _request._content.size());
}

void HttpDownload::onRecvContent(const char *data, uint64_t len)
{
    if (!data || len == 0) {
        return ;
    }
    
    _recvSize += len;

    if (_chunkedParser) {
        _chunkedParser->parse(data, len);
        
        if (_parser._contentLen <= 0) {
            close();
        }
        return ;
    }
    
    onHttpResponce(data, len);

    if (_parser._contentLen <= 0) {
        close();
    }
}

void HttpDownload::close()
{
    if (_onClose) {
        _onClose(_fileSize);
    }

    HttpClient::close();
}

void HttpDownload::onError(const string& err)
{
    logInfo << "get a error: " << err;

    close();
}

void HttpDownload::setOnHttpResponce(const function<void(const char* data, int len)>& cb)
{
    _onHttpResponce = cb;
}

void HttpDownload::setOnClose(const function<void(uint64_t filesize)>& cb)
{
    _onClose = cb;
}

void HttpDownload::onHttpResponce(const char* data, int len)
{
    if (_onHttpResponce) {
        _onHttpResponce(data, len);
    }

    if (_parser._contentLen <= 0) {
        close();
    }
}