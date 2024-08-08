#ifndef GB28181Parser_H
#define GB28181Parser_H

#include "Buffer.h"
#include <unordered_map>

using namespace std;

class GB28181Parser {
public:
    void parse(const char *data, size_t len);

    void setOnRtpPacket(const function<void(const char* data, int len)>& cb);
    void onRtpPacket(const char* data, int len);

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
    function<void(const char* data, int len)> _onRtpPacket;
};



#endif //GB28181Parser_H
