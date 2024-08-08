#include "HttpChunkedParser.h"
#include "Logger.h"
#include "Util/String.h"

#include <algorithm>

using namespace std;

HttpChunkedParser::HttpChunkedParser()
{}

HttpChunkedParser::~HttpChunkedParser()
{
}


void HttpChunkedParser::parse(const char *data, size_t len)
{
    // 从配置中获取
    static int maxRemainSize = 4 * 1024 * 1024;

    int remainSize = _remainData.size();
    if (remainSize > maxRemainSize) {
        logError << "remain cache is too large";
        _remainData.clear();
        return ;
    }

    if (remainSize > 0) {
        _remainData.append(data, len);
        data = _remainData.data();
        len = _remainData.size();
    }

    logInfo << "_contentLen: " << _contentLen 
            << ", _state: " << (int)_stage
            << ", remainSize: " << (int)remainSize
            << ", len: " << (int)len;
    // logInfo << "buffer : " << string(data, len);
    // 说明上一次没有收全content
    // if (_stage == 2) {
    //     if (len < _contentLen) {
    //         // 数据还是不够
    //         // int left = _contentLen - len;
    //         logInfo << "on http body";
    //         if (_contentLen > 2) {
    //             if (len < _contentLen - 2) {
    //                 onHttpBody(data, len);
    //             } else {
    //                 onHttpBody(data, _contentLen - 2);
    //             }
    //         }
    //         _contentLen -= len;
    //         _remainData.clear();
    //         return ;
    //     } else {
    //         // 解析content
    //         // _content = string(data, _contentLen);
    //         if (_contentLen > 2) {
    //             onHttpBody(data, _contentLen - 2);
    //         }
    //         data += _contentLen;
    //         len -= _contentLen;
    //         _contentLen = 0;
    //         logInfo << "on http body";
    //         // onHttpRequest();
    //         _stage = 1;

    //         if (len == 0) {
    //             _remainData.clear();
    //             return ;
    //         }
    //     }
    // }

    auto start = data;
    auto end = data + len;
    
    while (data < end) {
        if (_stage == 1) {
            auto pos = strstr(data,"\r\n");
            if(pos == nullptr){
                logInfo << "pos == nullptr";
                break;
            }
            // handle request line
            string sizeStr(data, pos - data);
            sscanf(sizeStr.data(),"%X",&_contentLen);
            logInfo << "_contentLen: " << _contentLen;
            _contentLen += 2;
            _stage = 2;
            data = pos + 2;
        } else if (_stage == 2) {
            if (end - data < _contentLen) {
                break;
            } else {
                logInfo << "onHttpBody: " << (_contentLen - 2);
                if (_contentLen > 2) {
                    onHttpBody(data, _contentLen - 2);
                }
                data += _contentLen;
                _stage = 1;
                _contentLen = 0;
            }
        }
    }

    if (data < end) {
        // logInfo << "have remain data";
        _remainData.assign(data, end - data);
    } else {
        // logInfo << "don't have remain data";
        _remainData.clear();
    }
}

void HttpChunkedParser::setOnHttpBody(const function<void(const char* data, int len)>& cb)
{
    logInfo << "setOnHttpBody";
    _onHttpBody = cb;
}

void HttpChunkedParser::onHttpBody(const char* data, int len)
{
    // logInfo << "_onHttpBody";
    if (_onHttpBody) {
        _onHttpBody(data, len);
    }
}
