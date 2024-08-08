#ifndef WebrtcParser_H
#define WebrtcParser_H

#include "Buffer.h"
#include <unordered_map>

using namespace std;

class WebrtcParser {
public:
    void parse(const char *data, size_t len);

    void setOnRtcPacket(const function<void(const char* data, int len)>& cb);
    void onRtcPacket(const char* data, int len);

public:
    int _contentLen = 0;
    string _content;
    string _method;
    string _url;
    string _version;
    unordered_map<string, string> _mapHeaders;

private:
    int _stage = 1; //1:size header, 2:payload
    StringBuffer _remainData;
    function<void(const char* data, int len)> _onRtcPacket;
};



#endif //GB28181Parser_H
