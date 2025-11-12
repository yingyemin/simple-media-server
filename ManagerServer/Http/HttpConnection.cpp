#include "HttpConnection.h"
#include "Common/Config.h"
#include "Log/Logger.h"
#include "HttpUtil.h"
#include "Util/String.hpp"

#include "Common/Define.h"
#include "Util/Base64.h"
#include "Ssl/SHA1.h"
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
}

void HttpConnection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    logTrace << "get a buf: " << buffer->size();
    _parser.parse(buffer->data(), buffer->size());
}

void HttpConnection::onError(const std::string& errMsg)
{
    close();
    logWarn << "get a error: " << errMsg;
}

void HttpConnection::onHttpError(const string& msg)
{
    HttpResponse rsp;
    rsp._status = 400;
    rsp.setContent(msg);
    writeHttpResponse(rsp);

    logWarn << "get a error: " << msg;
}

ssize_t HttpConnection::send(Buffer::Ptr pkt)
{
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
        // WebsocketFrame frame;
        // frame.finish = 1;
        // frame.rsv1 = 0;
        // frame.rsv2 = 0;
        // frame.rsv3 = 0;
        // frame.opcode = OpcodeType_BINARY;
        // frame.mask = 0;
        // frame.payloadLen = pkt->size();

        // auto header = make_shared<StringBuffer>();
        // _websocket.encodeHeader(frame, header);

        // TcpConnection::send(header);

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
                //{"GET", &HttpConnection::handleGet},
                //{"POST", &HttpConnection::handlePost},
                //{"PUT", &HttpConnection::handlePut},
                //{"DELETE", &HttpConnection::handleDelete},
                //{"HEAD", &HttpConnection::handleHead},
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
    if(_parser._mapHeaders["connection"] == "keep-alive"){
        logInfo << "CONNECTION IS keep-alive";
        rsp.setHeader("Connection","keep-alive");
    }
    else{
        rsp.setHeader("Connection","close");
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

    _parser.clear();
    if (!_isWebsocket) {
        _onHttpBody = nullptr;
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