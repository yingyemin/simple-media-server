#ifndef GB28181SIPParser_H
#define GB28181SIPParser_H

#include "Buffer.h"
#include <unordered_map>

using namespace std;

class GB28181SIPParser {
public:
    void parse(const char *data, size_t len);

    void setOnGB28181Packet(const function<void(const char* data, int len)>& cb);
    void onGB28181Packet(const char* data, int len);

public:
    int _contentLen = 0;
    string _content;
    string _method;
    string _url;
    string _version;
    unordered_map<string, string> _mapHeaders;

private:
    int _stage = 1; //1:handle request line, 2:handle header line, 3:handle content
    StringBuffer _remainData;
    function<void(const char* data, int len)> _onGB28181Packet;
};



#endif //GB28181SIPParser_H
