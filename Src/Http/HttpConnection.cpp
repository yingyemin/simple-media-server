#include "HttpConnection.h"
#include "Common/Config.h"
#include "Log/Logger.h"
#include "HttpUtil.h"
#include "Util/String.h"

#ifdef ENABLE_RTMP
#include "Rtmp/FlvMuxerWithRtmp.h"
#endif

#ifdef ENABLE_HLS
#include "Hls/HlsManager.h"
#include "Hls/LLHlsManager.h"
#endif
#include "Common/Define.h"
#include "Util/Base64.h"
#include "Ssl/SHA1.h"
#include "Common/HookManager.h"
#include "Common/ApiUtil.h"

using namespace std;

unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

HttpConnection::HttpConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket, bool enbaleSsl)
    :TcpConnection(loop, socket, enbaleSsl)
    ,_loop(loop)
    ,_socket(socket)
{
    logInfo << "HttpConnection: " << this;
    _clock.start();
}

HttpConnection::~HttpConnection()
{
    logInfo << "~HttpConnection: " << this;
    if (_onClose) {
        _onClose();
    }
}

void HttpConnection::init()
{
    weak_ptr<HttpConnection> wSelf = static_pointer_cast<HttpConnection>(shared_from_this());
    logTrace << this;
    _parser.setOnHttpRequest([wSelf](){
        logTrace << "HttpConnection setOnHttpRequest";
        auto self = wSelf.lock();
        if (self) {
            self->onHttpRequest();
        }
    });

    _parser.setOnHttpBody([wSelf](const char* data, int len){
        logTrace << "HttpConnection setOnHttpBody";
        auto self = wSelf.lock();
        if (self && self->_onHttpBody) {
            self->_onHttpBody(data, len);
        }
    });

    // 端口不用监听，不允许随便更改
    // static int apiPort = Config::instance()->getAndListen([](const json& config){
    // _apiPort = Config::instance()->get("Http", "Api", _serverId,  "port");
    //     logInfo << "apiPort: " << apiPort;
    // }, "Http", "Api", "Api1",  "port");
}

void HttpConnection::close()
{
    TcpConnection::close();
}

void HttpConnection::onManager()
{
    logTrace << "manager: " << this;
    static int keepaliveTime = Config::instance()->getAndListen([](const json &config){
        keepaliveTime = Config::instance()->get("Http", "Server", "Server1", "keepaliveTime", "15");
    }, "Http", "Server", "Server1", "keepaliveTime", "15");

    HttpConnection::Wptr wSelf = dynamic_pointer_cast<HttpConnection>(shared_from_this());
    if (_clock.startToNow() > keepaliveTime * 1000) {
        logInfo << "manager: " << this << " keealive timeout";
        _loop->async([wSelf](){
            auto self = wSelf.lock();
            if (self) {
                self->close();
            }
        }, false);
    }
}

void HttpConnection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    logTrace << "get a buf: " << buffer->size();
    _clock.update();
    _parser.parse(buffer->data(), buffer->size());
}

void HttpConnection::onError()
{
    close();
    logWarn << "get a error: ";
}

void HttpConnection::onError(const string& msg)
{
    HttpResponse rsp;
    rsp._status = 400;
    rsp.setContent(msg);
    writeHttpResponse(rsp);

    logWarn << "get a error: " << msg;
}

ssize_t HttpConnection::send(Buffer::Ptr pkt)
{
    _clock.update();
    _totalSendBytes += pkt->size();
    _intervalSendBytes += pkt->size();
    // logInfo << "pkt size: " << pkt->size();
    if (_isChunked) {
        std::stringstream ss;
        ss << hex << pkt->size();
        string sizeStr = ss.str();

        auto sizeBuffer = make_shared<StringBuffer>();
        sizeBuffer->assign(sizeStr.data(), sizeStr.size());
        sizeBuffer->append("\r\n");
        TcpConnection::send(sizeBuffer);
        TcpConnection::send(pkt);
        
        auto lfcf = make_shared<StringBuffer>();
        lfcf->assign("\r\n");
        TcpConnection::send(lfcf);

        return sizeStr.size() + 4 + pkt->size();
    }

    if (_isWebsocket) {
        WebsocketFrame frame;
        frame.finish = 1;
        frame.rsv1 = 0;
        frame.rsv2 = 0;
        frame.rsv3 = 0;
        frame.opcode = OpcodeType_BINARY;
        frame.mask = 0;
        frame.payloadLen = pkt->size();

        auto header = make_shared<StringBuffer>();
        _websocket.encodeHeader(frame, header);

        TcpConnection::send(header);

        // 因为此处不用做掩码，payload不用encode
        // if (pkt->size() > 0) {
        //     _websocket.encodePayload(frame, pkt);
        // }
    }
    
    return TcpConnection::send(pkt);
}

void HttpConnection::onHttpRequest()
{
    try {
        logDebug << "origin _parser._url: " << _parser._url;

        _parser._url = "http://" + _socket->getLocalIp() + ":" + to_string(_socket->getLocalPort())
                        + _parser._url;
        
        logDebug << "_parser._url: " << _parser._url;
        UrlParser tmp;
        _urlParser = tmp;
        _urlParser.parse(_parser._url);
        logDebug << _urlParser.path_;

        if (g_mapApi.find(_urlParser.path_) != g_mapApi.end() && _parser._method != "OPTIONS") {
            weak_ptr<HttpConnection> wSelf = static_pointer_cast<HttpConnection>(shared_from_this());
            _onHttpBody = [wSelf](const char* data, int len){
                auto self = wSelf.lock();
                if (!self) {
                    return ;
                }

                if (len > 0) {
                    self->_parser._content.append(data, len);
                }
                if (self->_parser._contentLen > len) {
                    return ;
                }
                auto iter = self->_parser._mapHeaders.find("content-type");
                if (iter == self->_parser._mapHeaders.end()) {
                    logTrace << "no content-type";
                } else if (iter->second.find("application/json") != string::npos) {
                    logTrace << "self->_parser._content: " << self->_parser._content;
                    self->_parser._body = json::parse(self->_parser._content);
                } else if (iter->second == "application/x-www-form-urlencoded") {
                    auto body = split(self->_parser._content, "&", "=");
                    for (auto& iter : body) {
                        logTrace << "key: " << iter.first << ", value: " << iter.second;
                        self->_parser._body[iter.first] = iter.second;
                    }
                }

                for (auto& iter : self->_urlParser.vecParam_) {
                    self->_parser._body[iter.first] = iter.second;
                }

                self->apiRoute(self->_parser, self->_urlParser, [wSelf](HttpResponse& rsp){
                    auto self = wSelf.lock();
                    if (self) {
                        self->writeHttpResponse(rsp);
                    }
                });
            };
        } else if (startWith(_urlParser.path_, "/api/v1") && _parser._method != "OPTIONS") {
            HttpResponse rsp;
            rsp._status = 400;
            json value;
            value["msg"] = "unsupport api: " + _urlParser.path_;
            rsp.setContent(value.dump());
            writeHttpResponse(rsp);
        } else {
            auto method = _parser._method;
            static unordered_map<string, void (HttpConnection::*)()> httpHandle {
                {"GET", &HttpConnection::handleGet},
                {"POST", &HttpConnection::handlePost},
                {"PUT", &HttpConnection::handlePut},
                {"DELETE", &HttpConnection::handleDelete},
                {"HEAD", &HttpConnection::handleHead},
                {"OPTIONS", &HttpConnection::handleOptions}
            };

            // logInfo << _parser._method;
            auto it = httpHandle.find(_parser._method);
            if (it != httpHandle.end()) {
                (this->*(it->second))();
            } else {
                HttpResponse rsp;
                rsp._status = 400;
                json value;
                value["msg"] = "unsupport method: " + method;
                rsp.setContent(value.dump());
                writeHttpResponse(rsp);
            }                   
        }
    } catch (exception& ex) {
        HttpResponse rsp;
        rsp._status = 400;
        rsp.setContent(ex.what());
        writeHttpResponse(rsp);
    }
}

void HttpConnection::apiRoute(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    string msg = "unknwon api";
    auto it = g_mapApi.find(urlParser.path_);
    try {
        if (it != g_mapApi.end()) {
            it->second(parser, urlParser, rspFunc);
            return ;
        }
    } catch (ApiException& ex) {
        logInfo << urlParser.path_ << " error: " << ex.what();
        msg = ex.what();
    } catch (exception& ex) {
        logInfo << urlParser.path_ << " error: " << ex.what();
        msg = ex.what();
    } catch (...) {
        logInfo << urlParser.path_ << " error";
    }

    HttpResponse rsp;
    rsp._status = 400;
    json value;
    value["msg"] = msg;
    value["code"] = 400;
    rsp.setContent(value.dump());
    rspFunc(rsp);
}

void HttpConnection::writeHttpResponse(HttpResponse& rsp) // 将要素按照HttpResponse协议进行组织，再发送
{
    // 完善头部字段
    static int keepaliveTime = Config::instance()->getAndListen([](const json &config){
        keepaliveTime = Config::instance()->get("Http", "Server", "Server1", "keepaliveTime", "15");
    }, "Http", "Server", "Server1", "keepaliveTime", "15");

    bool bClose = false;
    if(_parser._mapHeaders["connection"] == "keep-alive" && rsp.getHeader("Connection") != "close"){
        logInfo << "CONNECTION IS keep-alive";
        rsp.setHeader("Connection","keep-alive");

        string keepAliveString = "timeout=";
        keepAliveString += keepaliveTime;
        keepAliveString += ", max=100";
        rsp.setHeader("Keep-Alive", std::move(keepAliveString));
    }
    else{
        rsp.setHeader("Connection","close");
        bClose = true;
    }
    if(!rsp._body.empty() && !rsp.hasHeader("Content-Length")){
        rsp.setHeader("Content-Length",std::to_string(rsp._body.size()));
    }
    if(!rsp._body.empty() && !rsp.hasHeader("Content-Type")){
        rsp.setHeader("Content-Type","application/octet-stream");
    }
    if(rsp._redirectFlag) {
        rsp.setHeader("Location",rsp._redirectUrl);
    }
    rsp.setHeader("Server", "SimpleMediaServer");
    rsp.setHeader("Access-Control-Allow-Origin", "*");
    rsp.setHeader("Access-Control-Allow-Credentials", "true");

    if (_isWebsocket) {
        rsp._status = 101;
        bClose = false;

        rsp._headers["Connection"] = "Upgrade";
        rsp.setHeader("Upgrade", "websocket");

        static string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        string key = _parser._mapHeaders["sec-websocket-key"];
        auto acceptStr = Base64::encode(SHA1::encode(key + magic));
        rsp.setHeader("Sec-WebSocket-Accept", acceptStr);

        if (_parser._mapHeaders.find("sec-websocket-protocol") != _parser._mapHeaders.end()) {
            rsp.setHeader("Sec-WebSocket-Protocol", _parser._mapHeaders["sec-websocket-protocol"]);
        }

        if (_parser._mapHeaders.find("sec-websocket-version") != _parser._mapHeaders.end()) {
            rsp.setHeader("Sec-WebSocket-Version", _parser._mapHeaders["sec-websocket-version"]);
        }
    }
    
    // 将rsp完善，按照http协议格式进行组织
    std::stringstream rsp_str;
    rsp_str << _parser._version << " " << std::to_string(rsp._status) << " " << HttpUtil::getStatusDesc(rsp._status) << "\r\n";
    for(auto &head: rsp._headers){
        rsp_str << head.first<<": " << head.second << "\r\n";
    }
    rsp_str << "\r\n";
    rsp_str << rsp._body;

    // logTrace << "rsp._body.size(): " << rsp._body.size();
    // logTrace << "rsp_str.str().size(): " << rsp_str.str().size();

    if (_parser._url.find("/api/v1") != string::npos) {
        logInfo << "url: " << _parser._url << "body: " << _parser._content << ", response: " << rsp_str.str() << ", this: " << this;
    } else {
        logInfo << "url: " << _parser._url;
    }

    // 发送数据
    auto buffer = StreamBuffer::create();
    buffer->assign(rsp_str.str().c_str(),rsp_str.str().size());
    // logInfo << "send rsp: " << rsp_str.str();
    TcpConnection::send(buffer);
    _clock.update();

    _parser.clear();
    if (!_isWebsocket) {
        _onHttpBody = nullptr;
    }

    if (bClose) {
        close();
    }
}

void HttpConnection::sendFile() // 将要素按照HttpResponse协议进行组织，再发送
{
    HttpResponse rsp;
    // 完善头部字段
    if(_parser._mapHeaders["connection"] == "keep-alive"){
        logDebug << "CONNECTION IS keep-alive";
        rsp.setHeader("Connection","keep-alive");
    }
    else{
        rsp.setHeader("Connection","close");
    }
    
    if(rsp._redirectFlag) {
        rsp.setHeader("Location",rsp._redirectUrl);
    }

    rsp.setHeader("Server", "SimpleMediaServer");
    rsp.setHeader("Access-Control-Allow-Origin", "*");
    rsp.setHeader("Access-Control-Allow-Credentials", "true");

    if (_isWebsocket) {
        rsp._status = 101;

        rsp._headers["Connection"] = "Upgrade";
        rsp.setHeader("Upgrade", "websocket");

        static string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        string key = _parser._mapHeaders["sec-websocket-key"];
        auto acceptStr = Base64::encode(SHA1::encode(key + magic));
        rsp.setHeader("Sec-WebSocket-Accept", acceptStr);

        if (_parser._mapHeaders.find("sec-websocket-protocol") != _parser._mapHeaders.end()) {
            rsp.setHeader("Sec-WebSocket-Protocol", _parser._mapHeaders["sec-websocket-protocol"]);
        }

        if (_parser._mapHeaders.find("sec-websocket-version") != _parser._mapHeaders.end()) {
            rsp.setHeader("Sec-WebSocket-Version", _parser._mapHeaders["sec-websocket-version"]);
        }
    }

    if (_httpFile) {
        rsp.setHeader("Content-Length", std::to_string(_httpFile->getSize()));
        rsp.setHeader("Content-Type", HttpUtil::getMimeType(_httpFile->getFilePath()));
        if (!_rangeStr.empty()) {
            rsp._status = 206;
            rsp.setHeader("Content-Range", "bytes " + _rangeStr + "/" + to_string(_httpFile->getFileSize()));
        }
    }
    
    // 将rsp完善，按照http协议格式进行组织
    std::stringstream rsp_str;
    rsp_str << _parser._version << " " << std::to_string(rsp._status) << " " << HttpUtil::getStatusDesc(rsp._status) << "\r\n";
    for(auto &head: rsp._headers){
        rsp_str << head.first<<": " << head.second << "\r\n";
    }
    rsp_str << "\r\n";

    weak_ptr<HttpConnection> wSelf = dynamic_pointer_cast<HttpConnection>(shared_from_this());
    _socket->setOnGetBuffer([wSelf](){
        logTrace << "setOnGetBuffer";
        auto self = wSelf.lock();
        if (!self) {
            return false;
        }

        logTrace << "read file";
        auto buffer = self->_httpFile->read();
        if (!buffer) {
            self->_socket->setOnGetBuffer(nullptr);
            // TODO 定时器停止socket？？
            return false;
        }

        self->send(buffer);
        return true;
    });
    
    // 发送Header
    auto buffer = StreamBuffer::create();
    buffer->assign(rsp_str.str().c_str(),rsp_str.str().size());
    logTrace << "send rsp: " << rsp_str.str();
    send(buffer);

    _parser.clear();
    if (!_isWebsocket) {
        _onHttpBody = nullptr;
    }
}

void HttpConnection::setFileRange(const string& rangeStr)
{
    // Range:bytes=0-1024,2000-3000
    // 目前只支持单个range，示例是两个range
    if (rangeStr.empty() || rangeStr == "-") {
        return ;
    }

    try {
        auto vec = split(rangeStr, "=");
        if (vec.size() == 1 || vec.size() == 2) {
            int index = vec.size() - 1;
            _rangeStr = vec[index];

            auto startStr = findSubStr(_rangeStr, "", "-");
            auto endStr = findSubStr(_rangeStr, "-", "");
            uint64_t fileSize = _httpFile->getFileSize();
            uint64_t startPos;
            uint64_t len;

            if (startStr.empty()) {
                len = stoull(endStr);
                if (len > fileSize) {
                    return ;
                }
                startPos = fileSize - len;
            } else {
                uint64_t endPos;
                if (endStr.empty()) {
                    endPos = fileSize;
                } else {
                    endPos = stoull(endStr);
                }

                startPos = stoull(startStr);

                if (endPos < startPos) {
                    return ;
                }

                if (endPos >= fileSize) {
                    endPos = fileSize - 1;
                }

                len = endPos - startPos + 1;
            }

            _httpFile->setRange(startPos, len);
        }
    } catch (...) {

    }
}

void HttpConnection::handleGet()
{
    weak_ptr<HttpConnection> wSelf = static_pointer_cast<HttpConnection>(shared_from_this());
    if (_parser._mapHeaders.find("sec-websocket-key") != _parser._mapHeaders.end()) {
        _isWebsocket = true;
        _websocket.setOnWebsocketFrame([wSelf](const char* data, int len){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            self->onWebsocketFrame(data, len);
        });

        _onHttpBody = [wSelf](const char* data, int len){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            self->_websocket.decode((unsigned char*)data, len);
        };
    }

    _parser._contentLen = 0;

    static int httpServerPort = Config::instance()->getAndListen([](const json &config){
        httpServerPort = Config::instance()->get("Http", "Server", "Server1", "port");
    }, "Http", "Server", "Server1", "port");

    if (_socket->getLocalPort() == httpServerPort) {
        static int interval = Config::instance()->getAndListen([](const json &config){
            interval = Config::instance()->get("Http", "Server", "Server1", "interval");
            if (interval == 0) {
                interval = 5000;
            }
        }, "Http", "Server", "Server1", "interval");

        if (interval == 0) {
            interval = 5000;
        }
        
        _loop->addTimerTask(interval, [wSelf](){
            auto self = wSelf.lock();
            if (!self) {
                return 0;
            }
            self->_lastBitrate = self->_intervalSendBytes / (interval / 1000.0);
            self->_intervalSendBytes = 0;

            return interval;
        }, nullptr);

        PlayInfo info;
        info.protocol = _urlParser.protocol_;
        info.type = _urlParser.type_;
        info.uri = _urlParser.path_;
        info.vhost = _urlParser.vhost_;

        auto hook = HookManager::instance()->getHook(MEDIA_HOOK);
        if (hook) {
            hook->onPlay(info, [wSelf](const PlayResponse &rsp){
                auto self = wSelf.lock();
                if (!self) {
                    return ;
                }

                if (!rsp.authResult) {
                    HttpResponse rsp;
                    rsp._status = 400;
                    json value;
                    value["msg"] = "path is not exist: " + self->_urlParser.path_;
                    rsp.setContent(value.dump());
                    self->writeHttpResponse(rsp);

                    return ;
                }

                if (endWith(self->_urlParser.path_, ".flv")) {
                    self->handleFlvStream();
                } else if (endWith(self->_urlParser.path_, "sms.m3u8")) {
                    self->handleSmsHlsM3u8();
                } else if (endWith(self->_urlParser.path_, ".ll.m3u8")) {
                    self->handleLLHlsM3u8();
                } else if (endWith(self->_urlParser.path_, ".m3u8")) {
                    self->handleHlsM3u8();
                } else if (endWith(self->_urlParser.path_, ".ts")) {
                    if (self->_urlParser.path_.find("_hls") != string::npos) {
                        self->handleHlsTs();
                    } else {
                        self->handleTs();
                    }
                } else if (endWith(self->_urlParser.path_, ".ps")) {
                    self->handlePs();
                } else if (endWith(self->_urlParser.path_, ".mp4")) {
                    if (self->_urlParser.path_.find("_hls") != string::npos) {
                        self->handleLLHlsTs();
                    } else {
                        self->handleFmp4();
                    }
                }
            });
        } else {
            if (endWith(_urlParser.path_, ".flv")) {
                handleFlvStream();
            } else if (endWith(_urlParser.path_, "sms.m3u8")) {
                handleSmsHlsM3u8();
            } else if (endWith(_urlParser.path_, ".ll.m3u8")) {
                handleLLHlsM3u8();
            } else if (endWith(_urlParser.path_, ".m3u8")) {
                handleHlsM3u8();
            } else if (endWith(_urlParser.path_, ".ts")) {
                if (_urlParser.path_.find("_hls") != string::npos) {
                    handleHlsTs();
                } else {
                    handleTs();
                }
            } else if (endWith(_urlParser.path_, ".ps")) {
                handlePs();
            } else if (endWith(_urlParser.path_, ".mp4")) {
                if (_urlParser.path_.find("_hls") != string::npos) {
                    handleLLHlsTs();
                } else {
                    handleFmp4();
                }
            }
        }
    } else {
        logTrace << "download file or dir: " << _urlParser.path_;
        _httpFile = make_shared<HttpFile>(_parser);
        if (!_httpFile->isValid()) {
            HttpResponse rsp;
            rsp._status = 400;
            json value;
            value["msg"] = "invalid path: " + _urlParser.path_;
            rsp.setContent(value.dump());
            writeHttpResponse(rsp);
        } else {
            if (_httpFile->isFile()) {
                auto rangeIter = _parser._mapHeaders.find("range");
                if (rangeIter != _parser._mapHeaders.end()) {
                    setFileRange(rangeIter->second);
                }
                sendFile();
                logTrace << "send a file: " << _httpFile->getFilePath();
            } else if (_httpFile->isDir()) {
                auto indexStr = _httpFile->getIndex();

                HttpResponse rsp;
                rsp._status = 200;
                // json value;
                // value["msg"] = "unsupport path: " + _urlParser.path_;
                rsp.setContent(indexStr);
                writeHttpResponse(rsp);
                logTrace << "send a dir: " << _httpFile->getFilePath();
                logTrace <<"_parser _stage: " << _parser.getStage();
            } else {
                HttpResponse rsp;
                rsp._status = 400;
                json value;
                value["msg"] = "path is not exist: " + _urlParser.path_;
                rsp.setContent(value.dump());
                writeHttpResponse(rsp);
            }
        }

        // HttpResponse rsp;
        // rsp._status = 400;
        // json value;
        // value["msg"] = "unsupport path: " + _urlParser.path_;
        // rsp.setContent(value.dump());
        // writeHttpResponse(rsp);
    }
}

void HttpConnection::handlePost()
{}

void HttpConnection::handlePut()
{}

void HttpConnection::handleDelete()
{}

void HttpConnection::handleHead()
{}

void HttpConnection::handleOptions()
{
    logTrace << "HttpConnection::handleOptions()";

    HttpResponse rsp;
    rsp._status = 200;
    rsp.setHeader("Access-Control-Allow-Headers", "origin,range,accept-encoding,referer,Cache-Control,X-Proxy-Authorization,X-Requested-With,Content-Type");
    rsp.setHeader("Access-Control-Allow-Methods", "GET, POST, HEAD, PUT, DELETE, OPTIONS");
    rsp.setHeader("Access-Control-Allow-Origin", "*");
    rsp.setHeader("Access-Control-Expose-Headers", "Server,range,Content-Length,Content-Range");
    writeHttpResponse(rsp);
}

void HttpConnection::handleFlvStream()
{
#ifdef ENABLE_RTMP
    HttpResponse rsp;
    rsp._status = 200;
    rsp.setHeader("Content-Type", HttpUtil::getMimeType(_urlParser.path_));
    rsp.setHeader("Connection", "keep-alive");
    writeHttpResponse(rsp);

    auto flvMux = make_shared<FlvMuxerWithRtmp>(_urlParser, _socket->getLoop());
    weak_ptr<HttpConnection> wSelf = dynamic_pointer_cast<HttpConnection>(shared_from_this());
    logTrace << "flv mux set onwrite";
    flvMux->setOnWrite([wSelf](const char *data, int len){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        auto buffer = make_shared<StreamBuffer>(data, len);
        self->send(buffer);
    });
    
    flvMux->setOnWrite([wSelf](const StreamBuffer::Ptr& buffer){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        self->send(buffer);
    });

    logTrace << "flv mux setOnDetach";
    flvMux->setOnDetach([wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        self->close();
    });

    logTrace << "flv mux start";
    flvMux->setLocalIp(_socket->getLocalIp());
    flvMux->setLocalPort(_socket->getLocalPort());
    flvMux->setPeerIp(_socket->getPeerIp());
    flvMux->setPeerPort(_socket->getPeerPort());
    flvMux->start();

    logTrace << "oncolse";
    _onClose = [flvMux](){
        flvMux->setOnDetach(nullptr);
    };
#else
    HttpResponse rsp;
    rsp._status = 400;
    json value;
    value["msg"] = "rtmp not enabled";
    rsp.setContent(value.dump());
    writeHttpResponse(rsp);
#endif
}

void HttpConnection::handleSmsHlsM3u8()
{
#ifdef ENABLE_HLS
    string path = _urlParser.path_;
    trimBack(path, ".sms.m3u8");
    auto pos = path.find_last_of("/");
    auto streamName = path.substr(pos + 1);

    stringstream ss;
    ss << "#EXTM3U\n"
	   << "#EXT-X-VERSION:3\n"
       << "#EXT-X-ALLOW-CACHE:NO\n"
	   << "#EXT-X-TARGETDURATION:" << 1 /*maxDuration / 1000.0*/ << "\n"
	   << "#EXT-X-MEDIA-SEQUENCE:" << 0 << "\n"
       << "#EXTINF:" << 1 << "\n"
       << streamName << ".ts\n"
       << "#EXTINF:" << 1 << "\n"
       << streamName << ".ts\n"
       << "#EXTINF:" << 1 << "\n"
       << streamName << ".ts\n";

    logDebug << "ts path : " << streamName;

    HttpResponse rsp;
    rsp._status = 200;
    rsp.setHeader("Content-Type", HttpUtil::getMimeType(_urlParser.path_));
    // rsp.setHeader("Connection", "keep-alive");
    rsp.setContent(ss.str());
    writeHttpResponse(rsp);
#else
    HttpResponse rsp;
    rsp._status = 400;
    json value;
    value["msg"] = "hls not enabled";
    rsp.setContent(value.dump());
    writeHttpResponse(rsp);
#endif
}

void HttpConnection::handleHlsM3u8()
{
#ifdef ENABLE_HLS
    weak_ptr<HttpConnection> wSelf = dynamic_pointer_cast<HttpConnection>(shared_from_this());

    if (_urlParser.vecParam_.find("uid") != _urlParser.vecParam_.end()) {
        string path = _urlParser.path_;
        trimBack(path, ".m3u8");
        
        string strM3u8;
        auto uid = stoi(_urlParser.vecParam_["uid"]);
        string streamKey = path + "_" + _urlParser.vhost_ + "_" + _urlParser.type_;
        auto hlsMuxer = HlsManager::instance()->getMuxer(streamKey);
        if (!hlsMuxer) {
            logInfo << "find hls muxer by streamKey(" << streamKey << ") failed";
            strM3u8 = "source is not exists";
        } else {
            strM3u8 = hlsMuxer->getM3u8WithUid(uid);
        }
        HttpResponse rsp;
        rsp._status = 200;
        rsp.setHeader("Content-Type", HttpUtil::getMimeType(_urlParser.path_));
        // rsp.setHeader("Connection", "keep-alive");
        rsp.setContent(strM3u8);
        writeHttpResponse(rsp);

        logDebug << "m3u8 response: " << strM3u8;

        return ;
    }

    _mimeType = HttpUtil::getMimeType(_urlParser.path_);
	_urlParser.path_ = trimBack(_urlParser.path_, ".m3u8");
    MediaSource::getOrCreateAsync(_urlParser.path_, _urlParser.vhost_, _urlParser.protocol_, _urlParser.type_, 
    [wSelf](const MediaSource::Ptr &src){
        logTrace << "get a src";
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

		auto hlsSrc = dynamic_pointer_cast<HlsMediaSource>(src);
		if (!hlsSrc) {
			self->onError("hls source is not exist");
            return ;
		}

        // self->_source = rtmpSrc;

		self->_loop->async([wSelf, hlsSrc](){
			auto self = wSelf.lock();
			if (!self) {
				return ;
			}

			self->onPlayHls(hlsSrc);
		}, true);
    }, 
    [wSelf]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }
        return make_shared<HlsMediaSource>(self->_urlParser, nullptr, true);
    }, this);
#else
    HttpResponse rsp;
    rsp._status = 400;
    json value;
    value["msg"] = "hls not enabled";
    rsp.setContent(value.dump());
    writeHttpResponse(rsp);
#endif
}

#ifdef ENABLE_HLS
void HttpConnection::onPlayHls(const HlsMediaSource::Ptr &hlsSrc)
{
    weak_ptr<HttpConnection> wSelf = dynamic_pointer_cast<HttpConnection>(shared_from_this());

    hlsSrc->addOnReady(this, [wSelf, hlsSrc, this](){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        auto strM3u8 = hlsSrc->getM3u8(this);

        HttpResponse rsp;
        rsp._status = 200;
        rsp.setHeader("Content-Type", _mimeType);
        // rsp.setHeader("Connection", "keep-alive");
        rsp.setContent(strM3u8);
        logTrace << "write response";
        self->writeHttpResponse(rsp);

        logInfo << "m3u8 response: " << strM3u8;
    });

    // _onClose = [hlsSrc, this](){
    //     hlsSrc->delConnection(this);
    // };
}
#endif

void HttpConnection::handleHlsTs()
{
#ifdef ENABLE_HLS
    // if (_urlParser.vecParam_.find("uid") == _urlParser.vecParam_.end()) {
    //     HttpResponse rsp;
    //     rsp._status = 404;
    //     rsp.setHeader("Content-Type", HttpUtil::getMimeType(_urlParser.path_));
    //     // rsp.setHeader("Connection", "keep-alive");
    //     rsp.setContent("uid not exist");
    //     writeHttpResponse(rsp);

    //     return ;
    // }

    auto pos = _urlParser.path_.find_last_of("_");
    auto path = _urlParser.path_.substr(0, pos);

    auto hlsMuxer = HlsManager::instance()->getMuxer(path + "_" + DEFAULT_VHOST + "_" + DEFAULT_TYPE);
    string tsString;
    if (hlsMuxer) {
        auto tsBuffer = hlsMuxer->getTsBuffer(_urlParser.path_);
        if (tsBuffer) {
            tsString.assign(tsBuffer->data(), tsBuffer->size());
            logInfo << "find ts: " << _urlParser.path_;

            // auto pos = _urlParser.path_.find_last_of("/");
            // FILE* fp = fopen(_urlParser.path_.substr(pos + 1).data(), "wb");
            // fwrite(tsBuffer->data(), tsBuffer->size(), 1, fp);
            // fclose(fp);
        } else {
            logWarn << "ts is empty: " << _urlParser.path_;
        }
    }

    HttpResponse rsp;
    rsp._status = 200;
    rsp.setHeader("Content-Type", HttpUtil::getMimeType(_urlParser.path_));
    // rsp.setHeader("Connection", "keep-alive");
    rsp.setContent(tsString);
    writeHttpResponse(rsp);
#else
    HttpResponse rsp;
    rsp._status = 400;
    json value;
    value["msg"] = "hls not enabled";
    rsp.setContent(value.dump());
    writeHttpResponse(rsp);
#endif
}

void HttpConnection::handleLLHlsM3u8()
{
#ifdef ENABLE_HLS 
    weak_ptr<HttpConnection> wSelf = dynamic_pointer_cast<HttpConnection>(shared_from_this());

    if (_urlParser.vecParam_.find("uid") != _urlParser.vecParam_.end()) {
        string strM3u8;
        auto uid = stoi(_urlParser.vecParam_["uid"]);
        string streamKey = _urlParser.path_ + "_" + _urlParser.vhost_ + "_" + _urlParser.type_;
        auto hlsMuxer = LLHlsManager::instance()->getMuxer(streamKey);
        if (!hlsMuxer) {
            logInfo << "find hls muxer by uid(" << uid << ") failed";
            strM3u8 = "source is not exists";
        } else {
            strM3u8 = hlsMuxer->getM3u8WithUid(uid);
        }
        HttpResponse rsp;
        rsp._status = 200;
        rsp.setHeader("Content-Type", HttpUtil::getMimeType(_urlParser.path_));
        // rsp.setHeader("Connection", "keep-alive");
        rsp.setContent(strM3u8);
        writeHttpResponse(rsp);

        logInfo << "m3u8 response: " << strM3u8;

        return ;
    }

    _mimeType = HttpUtil::getMimeType(_urlParser.path_);
	_urlParser.path_ = trimBack(_urlParser.path_, ".ll.m3u8");
    MediaSource::getOrCreateAsync(_urlParser.path_, _urlParser.vhost_, _urlParser.protocol_, _urlParser.type_, 
    [wSelf](const MediaSource::Ptr &src){
        logTrace << "get a src";
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

		auto hlsSrc = dynamic_pointer_cast<LLHlsMediaSource>(src);
		if (!hlsSrc) {
			self->onError("hls source is not exist");
            return ;
		}

        // self->_source = rtmpSrc;

		self->_loop->async([wSelf, hlsSrc](){
			auto self = wSelf.lock();
			if (!self) {
				return ;
			}

			self->onPlayLLHls(hlsSrc);
		}, true);
    }, 
    [wSelf]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }
        return make_shared<LLHlsMediaSource>(self->_urlParser, nullptr, true);
    }, this);
#else
    HttpResponse rsp;
    rsp._status = 400;
    json value;
    value["msg"] = "hls not enabled";
    rsp.setContent(value.dump());
    writeHttpResponse(rsp);
#endif
}

#ifdef ENABLE_HLS
void HttpConnection::onPlayLLHls(const LLHlsMediaSource::Ptr &hlsSrc)
{
    weak_ptr<HttpConnection> wSelf = dynamic_pointer_cast<HttpConnection>(shared_from_this());

    hlsSrc->addOnReady(this, [wSelf, hlsSrc, this](){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        auto strM3u8 = hlsSrc->getM3u8(this);

        HttpResponse rsp;
        rsp._status = 200;
        rsp.setHeader("Content-Type", _mimeType);
        // rsp.setHeader("Connection", "keep-alive");
        rsp.setContent(strM3u8);
        logTrace << "write response";
        self->writeHttpResponse(rsp);

        logInfo << "m3u8 response: " << strM3u8;
    });

    _onClose = [hlsSrc, this](){
        hlsSrc->delConnection(this);
    };
}
#endif

void HttpConnection::handleLLHlsTs()
{
#ifdef ENABLE_HLS
    // if (_urlParser.vecParam_.find("uid") == _urlParser.vecParam_.end()) {
    //     HttpResponse rsp;
    //     rsp._status = 404;
    //     rsp.setHeader("Content-Type", HttpUtil::getMimeType(_urlParser.path_));
    //     // rsp.setHeader("Connection", "keep-alive");
    //     rsp.setContent("uid not exist");
    //     writeHttpResponse(rsp);

    //     return ;
    // }

    auto pos = _urlParser.path_.find_last_of("_");
    auto path = _urlParser.path_.substr(0, pos);

    auto hlsMuxer = LLHlsManager::instance()->getMuxer(path + "_" + DEFAULT_VHOST + "_" + DEFAULT_TYPE);
    string tsString;
    if (hlsMuxer) {
        auto tsBuffer = hlsMuxer->getTsBuffer(_urlParser.path_);
        if (tsBuffer) {
            tsString.assign(tsBuffer->data(), tsBuffer->size());
            logInfo << "find ts: " << _urlParser.path_;

            // auto pos = _urlParser.path_.find_last_of("/");
            // FILE* fp = fopen(_urlParser.path_.substr(pos + 1).data(), "wb");
            // fwrite(tsBuffer->data(), tsBuffer->size(), 1, fp);
            // fclose(fp);
        } else {
            logWarn << "ts is empty: " << _urlParser.path_;
        }
    }

    HttpResponse rsp;
    rsp._status = 200;
    rsp.setHeader("Content-Type", HttpUtil::getMimeType(_urlParser.path_));
    // rsp.setHeader("Connection", "keep-alive");
    rsp.setContent(tsString);
    writeHttpResponse(rsp);
#else
    HttpResponse rsp;
    rsp._status = 400;
    json value;
    value["msg"] = "hls not enabled";
    rsp.setContent(value.dump());
    writeHttpResponse(rsp);
#endif
}

void HttpConnection::handleTs()
{
#ifdef ENABLE_MPEG
    HttpResponse rsp;
    rsp._status = 200;
    rsp.setHeader("Content-Type", HttpUtil::getMimeType(_urlParser.path_));
    rsp.setHeader("Connection", "keep-alive");
    writeHttpResponse(rsp);

    _urlParser.path_ = trimBack(_urlParser.path_, ".ts");

    weak_ptr<HttpConnection> wSelf = dynamic_pointer_cast<HttpConnection>(shared_from_this());
    MediaSource::getOrCreateAsync(_urlParser.path_, _urlParser.vhost_, PROTOCOL_TS, _urlParser.type_, 
    [wSelf](const MediaSource::Ptr &src){
        logTrace << "get a src";
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

		auto tsSrc = dynamic_pointer_cast<TsMediaSource>(src);
		if (!tsSrc) {
			self->onError("hls source is not exist");
            return ;
		}

        // self->_source = rtmpSrc;

		self->_loop->async([wSelf, tsSrc](){
			auto self = wSelf.lock();
			if (!self) {
				return ;
			}

			self->onPlayTs(tsSrc);
		}, true);
    }, 
    [wSelf]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }
        return make_shared<TsMediaSource>(self->_urlParser, nullptr, true);
    }, this);
#else
    HttpResponse rsp;
    rsp._status = 400;
    json value;
    value["msg"] = "ts not enabled";
    rsp.setContent(value.dump());
    writeHttpResponse(rsp);
#endif
}

#ifdef ENABLE_MPEG
void HttpConnection::onPlayTs(const TsMediaSource::Ptr &tsSrc)
{
    weak_ptr<HttpConnection> wSelf = dynamic_pointer_cast<HttpConnection>(shared_from_this());

    if (!_playTsReader) {
		logTrace << "set _playTsReader";
		_playTsReader = tsSrc->getRing()->attach(EventLoop::getCurrentLoop(), true);
		_playTsReader->setGetInfoCB([wSelf]() {
			auto self = wSelf.lock();
			ClientInfo ret;
			if (!self) {
				return ret;
			}
			ret.ip_ = self->_socket->getLocalIp();
			ret.port_ = self->_socket->getLocalPort();
			ret.protocol_ = PROTOCOL_TS;
            ret.bitrate_ = self->_lastBitrate;
			return ret;
		});
		_playTsReader->setDetachCB([wSelf]() {
			auto strong_self = wSelf.lock();
			if (!strong_self) {
				return;
			}
			// strong_self->shutdown(SockException(Err_shutdown, "rtsp ring buffer detached"));
			strong_self->onError("ring buffer detach");
		});
		logTrace << "setReadCB =================";
		_playTsReader->setReadCB([wSelf](const TsMediaSource::RingDataType &pack) {
			auto self = wSelf.lock();
			if (!self/* || pack->empty()*/) {
				return;
			}
			// logInfo << "send rtmp msg";
			auto pktList = *(pack.get());
			for (auto& pkt : pktList) {
                self->send(pkt);
				// uint8_t frame_type = (pkt->payload.get()[0] >> 4) & 0x0f;
				// uint8_t codec_id = pkt->payload.get()[0] & 0x0f;

				// FILE* fp = fopen("testflv.rtmp", "ab+");
				// fwrite(pkt->payload.get(), pkt->length, 1, fp);
				// fclose(fp);

				// logInfo << "frame_type : " << (int)frame_type << ", codec_id: " << (int)codec_id
				// 		<< "pkt->payload.get()[0]: " << (int)(pkt->payload.get()[0]);
				// self->sendMediaData(pkt->type_id, pkt->abs_timestamp, pkt->payload, pkt->length);
			}
		});

        _onClose = [tsSrc, this](){
            tsSrc->delConnection(this);
        };
	}
} 
#endif

void HttpConnection::handlePs()
{
#ifdef ENABLE_MPEG
    HttpResponse rsp;
    rsp._status = 200;
    rsp.setHeader("Content-Type", HttpUtil::getMimeType(_urlParser.path_));
    rsp.setHeader("Connection", "keep-alive");
    writeHttpResponse(rsp);

    _urlParser.path_ = trimBack(_urlParser.path_, ".ps");

    weak_ptr<HttpConnection> wSelf = dynamic_pointer_cast<HttpConnection>(shared_from_this());
    MediaSource::getOrCreateAsync(_urlParser.path_, _urlParser.vhost_, PROTOCOL_PS, _urlParser.type_, 
    [wSelf](const MediaSource::Ptr &src){
        logTrace << "get a src";
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

		auto psSrc = dynamic_pointer_cast<PsMediaSource>(src);
		if (!psSrc) {
			self->onError("hls source is not exist");
            return ;
		}

        // self->_source = rtmpSrc;

		self->_loop->async([wSelf, psSrc](){
			auto self = wSelf.lock();
			if (!self) {
				return ;
			}

			self->onPlayPs(psSrc);
		}, true);
    }, 
    [wSelf]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }
        return make_shared<PsMediaSource>(self->_urlParser, nullptr, true);
    }, this);
#else
    HttpResponse rsp;
    rsp._status = 400;
    json value;
    value["msg"] = "ps not enabled";
    rsp.setContent(value.dump());
    writeHttpResponse(rsp);
#endif
}

#ifdef ENABLE_MPEG
void HttpConnection::onPlayPs(const PsMediaSource::Ptr &psSrc)
{
    weak_ptr<HttpConnection> wSelf = dynamic_pointer_cast<HttpConnection>(shared_from_this());

    if (!_playPsReader) {
		logTrace << "set _playPsReader";
		_playPsReader = psSrc->getRing()->attach(EventLoop::getCurrentLoop(), true);
		_playPsReader->setGetInfoCB([wSelf]() {
			auto self = wSelf.lock();
			ClientInfo ret;
			if (!self) {
				return ret;
			}
			ret.ip_ = self->_socket->getLocalIp();
			ret.port_ = self->_socket->getLocalPort();
			ret.protocol_ = PROTOCOL_PS;
            ret.bitrate_ = self->_lastBitrate;
			return ret;
		});
		_playPsReader->setDetachCB([wSelf]() {
			auto strong_self = wSelf.lock();
			if (!strong_self) {
				return;
			}
			// strong_self->shutdown(SockException(Err_shutdown, "rtsp ring buffer detached"));
			strong_self->onError("ring buffer detach");
		});
		logTrace << "setReadCB =================";
		_playPsReader->setReadCB([wSelf](const PsMediaSource::RingDataType &pack) {
			auto self = wSelf.lock();
			if (!self/* || pack->empty()*/) {
				return;
			}
			// logInfo << "send rtmp msg";
			auto pktList = *(pack.get());
			for (auto& pkt : pktList) {
                auto buffer = StreamBuffer::create();
                buffer->assign(pkt->data(), pkt->size());
                self->send(buffer);
				// uint8_t frame_type = (pkt->payload.get()[0] >> 4) & 0x0f;
				// uint8_t codec_id = pkt->payload.get()[0] & 0x0f;

				// FILE* fp = fopen("testflv.rtmp", "ab+");
				// fwrite(pkt->payload.get(), pkt->length, 1, fp);
				// fclose(fp);

				// logInfo << "frame_type : " << (int)frame_type << ", codec_id: " << (int)codec_id
				// 		<< "pkt->payload.get()[0]: " << (int)(pkt->payload.get()[0]);
				// self->sendMediaData(pkt->type_id, pkt->abs_timestamp, pkt->payload, pkt->length);
			}
		});
        _onClose = [psSrc, this](){
            psSrc->delConnection(this);
        };
	}
} 
#endif

void HttpConnection::handleFmp4()
{
#ifdef ENABLE_MP4
    HttpResponse rsp;
    rsp._status = 200;
    rsp.setHeader("Content-Type", HttpUtil::getMimeType(_urlParser.path_));
    rsp.setHeader("Connection", "keep-alive");
    writeHttpResponse(rsp);

    _urlParser.path_ = trimBack(_urlParser.path_, ".mp4");

    weak_ptr<HttpConnection> wSelf = dynamic_pointer_cast<HttpConnection>(shared_from_this());
    MediaSource::getOrCreateAsync(_urlParser.path_, _urlParser.vhost_, PROTOCOL_HTTP_FMP4, _urlParser.type_, 
    [wSelf](const MediaSource::Ptr &src){
        logTrace << "get a src";
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

		auto fmp4Src = dynamic_pointer_cast<Fmp4MediaSource>(src);
		if (!fmp4Src) {
			self->onError("fmp4 source is not exist");
            return ;
		}

        // self->_source = rtmpSrc;

		self->_loop->async([wSelf, fmp4Src](){
			auto self = wSelf.lock();
			if (!self) {
				return ;
			}

			self->onPlayFmp4(fmp4Src);
		}, true);
    }, 
    [wSelf]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }
        return make_shared<Fmp4MediaSource>(self->_urlParser, nullptr, true);
    }, this);
#else
    HttpResponse rsp;
    rsp._status = 400;
    json value;
    value["msg"] = "mp4 not enabled";
    rsp.setContent(value.dump());
    writeHttpResponse(rsp);
#endif
}

#ifdef ENABLE_MP4
void HttpConnection::onPlayFmp4(const Fmp4MediaSource::Ptr &fmp4Src)
{
    weak_ptr<HttpConnection> wSelf = dynamic_pointer_cast<HttpConnection>(shared_from_this());

    auto fmp4Header = fmp4Src->getFmp4Header();
    send(fmp4Header);

    if (!_playFmp4Reader) {
		logTrace << "set _playFmp4Reader";
		_playFmp4Reader = fmp4Src->getRing()->attach(EventLoop::getCurrentLoop(), true);
		_playFmp4Reader->setGetInfoCB([wSelf]() {
			auto self = wSelf.lock();
			ClientInfo ret;
			if (!self) {
				return ret;
			}
			ret.ip_ = self->_socket->getLocalIp();
			ret.port_ = self->_socket->getLocalPort();
			ret.protocol_ = PROTOCOL_PS;
            ret.bitrate_ = self->_lastBitrate;
			return ret;
		});
		_playFmp4Reader->setDetachCB([wSelf]() {
			auto strong_self = wSelf.lock();
			if (!strong_self) {
				return;
			}
			// strong_self->shutdown(SockException(Err_shutdown, "rtsp ring buffer detached"));
			strong_self->onError("ring buffer detach");
		});
		logTrace << "setReadCB =================";
		_playFmp4Reader->setReadCB([wSelf](const Fmp4MediaSource::RingDataType &pack) {
			auto self = wSelf.lock();
			if (!self/* || pack->empty()*/) {
				return;
			}
			// logInfo << "send fmp4 segment";
			// auto pktList = *(pack.get());
			// for (auto& pkt : pktList) {
            //     auto buffer = StreamBuffer::create();
            //     buffer->assign(pkt->data(), pkt->size());
                self->send(pack);
				// uint8_t frame_type = (pkt->payload.get()[0] >> 4) & 0x0f;
				// uint8_t codec_id = pkt->payload.get()[0] & 0x0f;

				// FILE* fp = fopen("testflv.rtmp", "ab+");
				// fwrite(pkt->payload.get(), pkt->length, 1, fp);
				// fclose(fp);

				// logInfo << "frame_type : " << (int)frame_type << ", codec_id: " << (int)codec_id
				// 		<< "pkt->payload.get()[0]: " << (int)(pkt->payload.get()[0]);
				// self->sendMediaData(pkt->type_id, pkt->abs_timestamp, pkt->payload, pkt->length);
			// }
		});
        _onClose = [fmp4Src, this](){
            fmp4Src->delConnection(this);
        };
	}
} 
#endif