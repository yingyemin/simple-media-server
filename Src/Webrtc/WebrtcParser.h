#ifndef WebrtcParser_H
#define WebrtcParser_H

#include "Buffer.h"
#include <unordered_map>

// using namespace std;

class WebrtcParser {
public:
    void parse(const char *data, size_t len);

    void setOnRtcPacket(const std::function<void(const char* data, int len)>& cb);
    void onRtcPacket(const char* data, int len);

public:
    int _contentLen = 0;
    std::string _content;
    std::string _method;
    std::string _url;
    std::string _version;
    std::unordered_map<std::string, std::string> _mapHeaders;

private:
    int _stage = 1; //1:size header, 2:payload
    StringBuffer _remainData;
    std::function<void(const char* data, int len)> _onRtcPacket;
};



#endif //GB28181Parser_H
