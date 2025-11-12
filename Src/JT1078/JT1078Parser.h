#ifndef JT1078Parser_H
#define JT1078Parser_H

#include "JT1078RtpPacket.h"
#include <unordered_map>

// using namespace std;

class JT1078Parser {
public:
    void parse(const char *data, size_t len);
    void parse2016(const char *data, size_t len);
    void parse2019(const char *data, size_t len);

    void setOnRtpPacket(const std::function<void(const JT1078RtpPacket::Ptr& buffer)>& cb);
    void onRtpPacket(const JT1078RtpPacket::Ptr& buffer);

private:
    int _stage = 1; //1:handle request line, 2:handle header line, 3:handle content
    int _simCodeSize = 6;
    JT1078_VERSION _version = JT1078_0;
    StringBuffer _remainData;
    std::function<void(const JT1078RtpPacket::Ptr& buffer)> _onRtpPacket;
};



#endif //JT1078Parser_H
