﻿#include "HttpParser.h"
#include "Logger.h"
#include "Util/String.h"

#include <algorithm>

using namespace std;

HttpParser::HttpParser()
{}

HttpParser::~HttpParser()
{
}


void HttpParser::parse(const char *data, size_t len)
{
    // 从配置中获取
    static int maxRemainSize = 4 * 1024 * 1024;

    int remainSize = _remainData.size();
    if (remainSize > maxRemainSize) {
        logError << "remain cache is too large";
        _remainData.clear();
        return ;
    }

    logTrace << "_remainData: " << _remainData.buffer() << ", url: " << _url;
    if (remainSize > 0) {
        _remainData.append(data, len);
        data = _remainData.data();
        len += remainSize;
    }

    // logInfo << "data string: " << string(data, len);
    logTrace << "_contentLen: " << _contentLen << ": " << this << ", url: " << _url;
    logTrace << "_stage: " << _stage << ", url: " << _url;
    // logInfo << "buffer : " << string(data, len);
    // 说明上一次没有收全content
    if (_stage == 3) {
        if (_contentLen == -1 || len < _contentLen) {
            // 数据还是不够
            if (_contentLen > 0) {
                _contentLen -= len;
            }
            logTrace << "on http body" << ", url: " << _url;
            onHttpBody(data, len);
            _remainData.clear();
            return ;
        } else {
            // 解析content
            // _content = string(data, _contentLen);
            int tmpLen = _contentLen;
            _contentLen = 0;
            onHttpBody(data, tmpLen);
            data += tmpLen;
            len -= tmpLen;
            logTrace << "on http body" << ", url: " << _url;
            // onHttpRequest();
            _stage = 1;

            if (len == 0) {
                _remainData.clear();
                return ;
            }
        }
    }

    auto start = data;
    auto end = data + len;

    // int crlfSize = 1;
    // char* crlf = "\n";
    // auto pos = strstr(data, "\n");
    // if (pos != nullptr && pos > data) {
    //     if (*(pos - 1) == '\r') {
    //         crlfSize = 2;
    //         crlf = "\r\n";
    //     }
    // }
    
    while (data < end) {
        auto pos = strstr(data,"\r\n");
        if(pos == nullptr){
            // logInfo << "pos == nullptr";
            break;
        }
        if (pos == data) {
            logTrace << "pos == data, _contentLen: " << _contentLen << ", url: " << _url;
            data += 2;
            if (_contentLen > 0 || _contentLen == -1) {
                onHttpRequest();
                logTrace << "_stage: " << _stage;
                if (_stage == 1) {
                    continue;
                }
                _stage = 3;
            } else {
                // 读完了头，且没有content。重新解析新的包
                // logInfo << "on http request";
                onHttpRequest();
                _contentLen = 0;
                onHttpBody(nullptr, 0);
                _stage = 1;
                continue;
            }
        }
        if (_stage == 1) {
            // handle request line
            auto blank = strchr(data, ' ');
            if (!(blank > data && blank < pos)) {
                _stage = 2;
                continue;
            }
            _method = std::string(data, blank - data);
            data = blank + 1;
            blank = strchr(data, ' ');
            if (!(blank > data && blank < pos)) {
                _stage = 2;
                continue;
            }
            _url = std::string(data, blank - data);
            _version = std::string(blank + 1, pos);
            _stage = 2;
        } else if (_stage == 2) {
            // logInfo << "_stage == 2";
            // handle hearder line

            auto keyPos = strchr(data, ':');
            if (!(keyPos > data && keyPos < pos)) {
                data = pos + 2;
                continue;
            }

            string key(data, keyPos - data);
            string value(keyPos + 1, pos - keyPos - 1);

            transform(key.begin(), key.end(), key.begin(), ::tolower);
            
            logTrace << "value is: " << value << ", url: " << _url;
            auto res = trim(value, " ");
            _mapHeaders[key] = res;
            if (key == "content-length") {
                _contentLen = stoi(value);
            }
        } else if (_stage == 3) {
            // logInfo << "_stage == 3";
            // handle content
            int leftSize = len - (data - start);
            // logInfo << "leftSize: " << leftSize;
            // logInfo << "_contentLen: " << _contentLen;
            if (_contentLen == -1 || leftSize < _contentLen) {
                logTrace << "on http body";
                if (_contentLen > 0) {
                    _contentLen -= leftSize;
                }
                onHttpBody(data, leftSize);
                data = end;
                break;
            } else {
                // 处理content
                _stage = 1;
                int tmpLen = _contentLen;
                _contentLen = 0;
                onHttpBody(data, tmpLen);
                data += tmpLen;
                // logInfo << "on http body";
                continue;
            }
        }
        data = pos + 2;
    }

    if (data < end) {
        logTrace << "have remain data" << ", url: " << _url;
        _remainData.assign(data, end - data);
    } else {
        logTrace << "don't have remain data" << ", url: " << _url;
        _remainData.clear();
    }

    logTrace << "_stage: " << _stage << ", url: " << _url; 
}

void HttpParser::setOnHttpRequest(const function<void()>& cb)
{
    logTrace << "setOnHttpRequest";
    _onHttpRequest = cb;
}

void HttpParser::onHttpRequest()
{
    logTrace << "onHttpRequest";
    if (_onHttpRequest) {
        _onHttpRequest();
    }
}

void HttpParser::setOnHttpBody(const function<void(const char* data, int len)>& cb)
{
    logTrace << "setOnHttpBody";
    _onHttpBody = cb;
}

void HttpParser::onHttpBody(const char* data, int len)
{
    // logInfo << "_onHttpBody";
    if (_onHttpBody) {
        _onHttpBody(data, len);
    }
}

void HttpParser::clear()
{
    // logTrace << "HttpParser::clear() : " << this;
    _contentLen = -1;
    _stage = 1;
    _content = "";
    // _method = "";
    // _url = "";
    // _version = "";
    // _body = nullptr;
    // _mapHeaders.clear();
    _remainData.clear();

    logTrace << "_stage: " << _stage;
}
