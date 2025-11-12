#ifndef RtspParser_H
#define RtspParser_H

#include "Buffer.h"
#include <unordered_map>

// using namespace std;

class RtspParser {
public:
    void parse(const char *data, size_t len);

    void setOnRtspPacket(const std::function<void()>& cb);
    void onRtspPacket();

    void setOnRtpPacket(const std::function<void(const char* data, int len)>& cb);
    void onRtpPacket(const char* data, int len);

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
    StreamBuffer::Ptr _rtpBuffer;
    std::function<void()> _onRtspPacket;
    std::function<void(const char* data, int len)> _onRtpPacket;
};



#endif //RtspSplitter_H
