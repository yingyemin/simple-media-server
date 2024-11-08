#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "GB28181SIPParser.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

void GB28181SIPParser::parse(const char *data, size_t len)
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
        len += remainSize;
    }

    auto start = data;
    auto end = data + len;
    auto startMsg = data;

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
            // logInfo << "pos == data";
            data += 2;
            if (_contentLen > 0) {
                _stage = 3;
            } else {
                // 读完了头，且没有content。重新解析新的包
                // logInfo << "on rtsp packet";
                onGB28181Packet(startMsg, data - startMsg);
                startMsg = data;
                _contentLen = 0;
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
                continue;
            }

            string key(data, keyPos - data);
            string value(keyPos + 1, pos - keyPos - 1);

            transform(key.begin(), key.end(), key.begin(), ::tolower);
            
            auto res = trim(value, " ");
            _mapHeaders[key] = res;
            logInfo << "key:" << key << ",value:" << res << ".";
            if (key == "content-length") {
                _contentLen = stoi(value);
            }
        } else if (_stage == 3) {
            // logInfo << "_stage == 3";
            // handle content
            int leftSize = len - (data - start);
            if (leftSize < _contentLen) {
                break;
            } else {
                // 处理content
                _stage = 1;
                _content = string(data, _contentLen);
                data += _contentLen;
                _contentLen = 0;
                // logInfo << "on rtsp packet";
                onGB28181Packet(startMsg, data - startMsg);
                startMsg = data;
                continue;
            }
        }
        data = pos + 2;
    }

    if (data < end) {
        // logInfo << "have remain data";
        _remainData.assign(data, end - data);
    } else {
        // logInfo << "don't have remain data";
        _remainData.clear();
    }
}

void GB28181SIPParser::setOnGB28181Packet(const function<void(const char* data, int len)>& cb)
{
    _onGB28181Packet = cb;
}

void GB28181SIPParser::onGB28181Packet(const char* data, int len)
{
    // logInfo << "on gb28181 packet";
    if (_onGB28181Packet) {
        _onGB28181Packet(data, len);
    }
}
