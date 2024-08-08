#include "HttpTsClient.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "Common/Define.h"

using namespace std;

HttpTsClient::HttpTsClient(MediaClientType type, const string& appName, const string& streamName)
    :HttpClient(EventLoop::getCurrentLoop())
    ,_type(type)
{
    _localUrlParser.path_ = "/" + appName + "/" + streamName;
    _localUrlParser.protocol_ = PROTOCOL_RTMP;
    _localUrlParser.type_ = DEFAULT_TYPE;
    _localUrlParser.vhost_ = DEFAULT_VHOST;
}

HttpTsClient::~HttpTsClient()
{
    auto tsSrc = _source.lock();
    if (tsSrc) {
        tsSrc->release();
        tsSrc->delConnection(this);
    }
}

void HttpTsClient::start(const string& localIp, int localPort, const string& url, int timeout)
{
    addHeader("Content-Type", "application/json;charset=UTF-8");
    setMethod("GET");
    logInfo << "connect to utl: " << url;
    sendHeader(url, timeout);
}

void HttpTsClient::stop()
{
    close();
}

void HttpTsClient::pause()
{

}

void HttpTsClient::onHttpRequest()
{
    // if(_parser._contentLen == 0){
    //     //后续没content，本次http请求结束
    //     close();
    // }

    if (_parser._mapHeaders["transfer-encoding"] == "chunked") {
        weak_ptr<HttpTsClient> wSelf = dynamic_pointer_cast<HttpTsClient>(shared_from_this());
        _chunkedParser = make_shared<HttpChunkedParser>();
        _chunkedParser->setOnHttpBody([wSelf](const char *data, int len){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            auto buffer = make_shared<StreamBuffer>();
            buffer->assign(data, len);
            auto psSource = self->_source.lock();
            if (psSource) {
                psSource->inputTs(buffer);
            }
        });
    }
}

void HttpTsClient::onConnect()
{
    _socket = TcpClient::getSocket();
    _loop = _socket->getLoop();

    HttpClient::onConnect();
    sendContent(_request._content.data(), _request._content.size());

    weak_ptr<HttpTsClient> wSelf = dynamic_pointer_cast<HttpTsClient>(shared_from_this());

    auto source = MediaSource::getOrCreate(_localUrlParser.path_, _localUrlParser.vhost_
                    , _localUrlParser.protocol_, _localUrlParser.type_
                    , [this]() {
                        return make_shared<TsMediaSource>(_localUrlParser, _loop);
                    });

    if (!source) {
        logWarn << "another stream is exist with the same uri";
        return ;
    }
    logInfo << "create a TsMediaSource";
    auto tsSource = dynamic_pointer_cast<TsMediaSource>(source);
    tsSource->setOrigin();

    auto tsDemuxer = make_shared<TsDemuxer>();

    tsSource->addTrack(tsDemuxer);
    _source = tsSource;
}

void HttpTsClient::onRecvContent(const char *data, uint64_t len) {
    // if (len == 0) {
    //     close();
    //     onHttpResponce();
    // }else if (_parser._contentLen == len) {
    //     _parser._content.append(data, len);
    //     close();
    //     onHttpResponce();
    // } else if (_parser._contentLen < len) {
    //     logInfo << "recv body len is bigger than conten-length";
    //     _parser._content.append(data, _parser._contentLen);
    //     close();
    //     onHttpResponce();
    // } else {
    //     _parser._content.append(data, len);
    // }

    if (_chunkedParser) {
        _chunkedParser->parse(data, len);
        return ;
    }

    auto buffer = make_shared<StreamBuffer>();
    buffer->assign(data, len);
    auto psSource = _source.lock();
    if (psSource) {
        psSource->inputTs(buffer);
    }
}

void HttpTsClient::close()
{
    if (_onClose) {
        _onClose();
    }

    HttpClient::close();
}

void HttpTsClient::onError(const string& err)
{
    logInfo << "get a error: " << err;

    close();
}

void HttpTsClient::setOnHttpResponce(const function<void(const HttpParser& parser)>& cb)
{
    _onHttpResponce = cb;
}

void HttpTsClient::onHttpResponce()
{
    if (_onHttpResponce) {
        _onHttpResponce(_parser);
    }
}

void HttpTsClient::setOnClose(const function<void()>& cb)
{
    _onClose = cb;
}