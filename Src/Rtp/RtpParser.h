#ifndef RtpParser_H
#define RtpParser_H

#include "Buffer.h"
#include <unordered_map>

// using namespace std;

class RtpParser {
public:
    void parse(const char *data, size_t len);

    void setOnRtpPacket(const std::function<void(const StreamBuffer::Ptr& buffer)>& cb);
    void onRtpPacket(const StreamBuffer::Ptr& buffer);

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
    StreamBuffer::Ptr _rtpBuffer;
    std::function<void(const StreamBuffer::Ptr& buffer)> _onRtpPacket;
};



#endif //RtpParser_H
