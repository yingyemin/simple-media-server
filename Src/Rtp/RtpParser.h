﻿#ifndef RtpParser_H
#define RtpParser_H

#include "Buffer.h"
#include <unordered_map>

using namespace std;

class RtpParser {
public:
    void parse(const char *data, size_t len);

    void setOnRtpPacket(const function<void(const StreamBuffer::Ptr& buffer)>& cb);
    void onRtpPacket(const StreamBuffer::Ptr& buffer);

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
    StreamBuffer::Ptr _rtpBuffer;
    function<void(const StreamBuffer::Ptr& buffer)> _onRtpPacket;
};



#endif //RtpParser_H
