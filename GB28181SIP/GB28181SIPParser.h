#ifndef GB28181SIPParser_H
#define GB28181SIPParser_H

#include "Buffer.h"
#include <unordered_map>

// using namespace std;

class GB28181SIPParser {
public:
    void parse(const char *data, size_t len);

    void setOnGB28181Packet(const std::function<void(const char* data, int len)>& cb);
    void onGB28181Packet(const char* data, int len);

public:
    int _contentLen = 0;
    std::string _content;
    std::string _method;
    std::string _url;
    std::string _version;
    std::unordered_map<std::string, std::string> _mapHeaders;

private:
    int _stage = 1; //1:handle request line, 2:handle header line, 3:handle content
    StringBuffer _remainData;
    std::function<void(const char* data, int len)> _onGB28181Packet;
};



#endif //GB28181SIPParser_H
