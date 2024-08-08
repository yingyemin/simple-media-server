#ifndef JT1078Parser_H
#define JT1078Parser_H

#include "JT1078RtpPacket.h"
#include <unordered_map>

using namespace std;

class JT1078Parser {
public:
    void parse(const char *data, size_t len);

    void setOnRtpPacket(const function<void(const JT1078RtpPacket::Ptr& buffer)>& cb);
    void onRtpPacket(const JT1078RtpPacket::Ptr& buffer);

private:
    int _stage = 1; //1:handle request line, 2:handle header line, 3:handle content
    StringBuffer _remainData;
    function<void(const JT1078RtpPacket::Ptr& buffer)> _onRtpPacket;
};



#endif //JT1078Parser_H
